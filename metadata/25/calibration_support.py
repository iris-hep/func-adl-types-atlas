"""R25 calibration support.

Starting with release 25, ATLAS's CP-Algorithms configuration is driven by the
block-based `AnalysisAlgorithmsConfig.ConfigText.TextConfig` API, which is meant
to be configured from a YAML document of blocks/options. Rather than modeling a
handful of calibration knobs in a `CalibrationEventConfig` dataclass and
rendering per-collection Jinja templates to build that config up piece by piece
(as the R21/R22 `calibration_support.py` still does), for R25 we instead:

* Ship a default, ready-to-load YAML config per dataset type
  (`config_phys.yaml` / `config_physlite.yaml`, alongside this file).
* Let a user override that default with their own YAML file
  (`calib_tools.set_calibration_yaml_path(...)`), giving them full, direct
  control over exactly what CP algorithms run - no calibration object, no
  release-specific metadata setup needed on our end.
* Render the chosen YAML with Jinja (the only genuinely per-query-dynamic bits
  are the systematic-error knobs in `CommonServices`) and embed the result as
  `add_job_script` metadata items, split one-per-block so that a query only
  pays for the blocks its collection accesses actually need (see
  `fixup_collection_call` and `CalibrationConfig.calibration_job_script_items`
  below) rather than the whole YAML unconditionally, the way an earlier
  version of this module worked. This is the same `add_job_script` mechanism
  already used to ship generated Python to the ServiceX backend today, so no
  backend change is required.
"""

import ast
import copy
import logging
import re
from pathlib import Path
from typing import Any, Dict, FrozenSet, List, Optional, Tuple, TypeVar

import jinja2
import yaml
from func_adl import ObjectStream
from func_adl.ast.meta_data import lookup_query_metadata

T = TypeVar("T")

_WIRING_TEMPLATE_NAME = "add_calibration_to_job.py"
_WIRING_TEMPLATE_PATH = Path(__file__).parent / "templates" / _WIRING_TEMPLATE_NAME

_PRIMARY_BLOCK_FOR_COLLECTION = {
    "jet_collection": "Jets",
    "electron_collection": "Electrons",
    "muon_collection": "Muons",
    "photon_collection": "Photons",
    "tau_collection": "TauJets",
    "met_collection": "MissingET",
}

# The five "real" reconstructed-object collection types. `MissingET` and
# `OverlapRemoval` are cross-object blocks built on top of these, not their
# own object type.
_OBJECT_COLLECTIONS: Tuple[str, ...] = (
    "jet_collection",
    "electron_collection",
    "muon_collection",
    "photon_collection",
    "tau_collection",
)

# collection_attr_name -> other object collections that its own block's YAML
# options reference as inputs, and therefore also needs configured before it
# can run. `MissingET`'s block references every object type as an input
# (`jets=`, `electrons=`, ...), same as `OverlapRemoval`'s.
_EXTRA_OBJECT_DEPENDENCIES: Dict[str, Tuple[str, ...]] = {
    "met_collection": _OBJECT_COLLECTIONS,
}

# Canonical block execution order (matches the sequence ATLAS's own
# ConfigText_unitTest.py uses): shared services first, then the five object
# types, then the cross-object blocks that consume them, then Thinning last
# (it trims containers for output, so needs to run after anything that still
# wants the full, untrimmed containers).
_CANONICAL_BLOCK_ORDER = [
    "CommonServices",
    "PileupReweighting",
    "Jets",
    "Electrons",
    "Muons",
    "Photons",
    "TauJets",
    "MissingET",
    "OverlapRemoval",
    "Thinning",
]


def _sanitize_identifier(name: str) -> str:
    "Make `name` safe to use as (part of) a python identifier."
    return re.sub(r"\W", "_", name)


def _resolve_needed_collections(
    accessed: FrozenSet[str],
) -> FrozenSet[str]:
    """Expand the raw set of directly-accessed `collection_attr_name`s with
    anything they require to be configured too (e.g. `met_collection` needs
    every object collection, since the `MissingET` block references all of
    them as inputs). Used to decide which *calibration blocks* must run -
    not which containers get thinned for output, which stays scoped to what
    was actually, directly accessed (see `CalibrationConfig.thinning_entries_for`).
    """
    needed = set(accessed)
    for name in accessed:
        needed.update(_EXTRA_OBJECT_DEPENDENCIES.get(name, ()))
    return frozenset(needed)


class CalibrationConfig:
    """A parsed calibration config.yaml.

    Everything in the parsed dict except the `_servicex_meta` key is a plain
    `TextConfig` block document (top-level keys are ATLAS config block names,
    e.g. `Jets`, `Electrons`; a block with sub-blocks nests them as plain dict
    keys, e.g. `Electrons: {WorkingPoint: {...}}`; a block invoked more than
    once, like `Thinning`, is a list of option-dicts). `_servicex_meta` is our
    own bookkeeping - calibrate/uncalibrated defaults and raw collection names -
    and is stripped out before anything is handed to `TextConfig`.
    """

    def __init__(self, name: str, raw_text: str):
        self.name = name
        self.raw_text = raw_text

    def render(self, sys_error: str) -> str:
        "Render this config's yaml for a particular systematic error choice."
        template = jinja2.Environment().from_string(self.raw_text)
        return template.render(sys_error=sys_error)

    def _parsed(self, sys_error: str) -> Dict[str, Any]:
        return yaml.safe_load(self.render(sys_error))

    def _meta(self) -> Dict[str, Any]:
        return self._parsed("NOSYS").get("_servicex_meta", {})

    def _blocks(self, sys_error: str) -> Dict[str, Any]:
        "The parsed config with `_servicex_meta` removed - what `TextConfig` gets."
        parsed = self._parsed(sys_error)
        parsed.pop("_servicex_meta", None)
        return parsed

    def calibrate_by_default(self) -> bool:
        return bool(self._meta().get("calibrate_by_default", True))

    def uncalibrated_possible(self) -> bool:
        return bool(self._meta().get("uncalibrated_possible", True))

    def raw_collection_name(self, collection_attr_name: str) -> str:
        """The un-calibrated xAOD container backing `collection_attr_name`
        (e.g. `jet_collection`) - used both as the default bank name to read
        from, and as what's returned for `calibrate=False`.
        """
        raw_names = self._meta().get("raw_collection_names", {})
        if collection_attr_name not in raw_names:
            raise NotImplementedError(
                f"Calibration config '{self.name}' does not define a "
                f"'_servicex_meta.raw_collection_names.{collection_attr_name}' "
                "entry - this collection type is not available."
            )
        return raw_names[collection_attr_name]

    def output_collection_name(
        self, collection_attr_name: str, sys_error: str
    ) -> str:
        """Resolve the final output collection name for `collection_attr_name`
        (e.g. `jet_collection`) out of this config's blocks.

        The primary block for the collection type (e.g. `Jets`) supplies the
        working container name. If a `Thinning` entry further narrows that
        container (as electrons/muons/photons/taus do), its `outputName` is
        used instead. The systematic-error name is always appended, matching
        the R21/R22 template convention this replaces.
        """
        block_name = _PRIMARY_BLOCK_FOR_COLLECTION.get(collection_attr_name)
        if block_name is None:
            raise ValueError(f"Unknown collection type '{collection_attr_name}'")

        blocks = self._blocks(sys_error)

        block = blocks.get(block_name)
        if block is None:
            raise NotImplementedError(
                f"No '{block_name}' block is configured for '{collection_attr_name}' "
                f"in calibration config '{self.name}' - this collection type is not "
                "available."
            )
        container_name = block.get("containerName")

        final_name = container_name
        thinning = blocks.get("Thinning", [])
        if isinstance(thinning, dict):
            thinning = [thinning]
        for entry in thinning:
            if entry.get("containerName") == container_name:
                final_name = entry.get("outputName", container_name)
                break

        return f"{final_name}_{sys_error}"

    def blocks_needed(
        self, needed_collections: FrozenSet[str], sys_error: str
    ) -> List[str]:
        """The top-level YAML block names that must be configured given
        `needed_collections` (already expanded via `_resolve_needed_collections`),
        in canonical execution order. `OverlapRemoval` is only pulled in once
        2+ object types are needed - there is nothing to remove overlaps
        between with just one - and any block is only included if this
        config's YAML actually defines it (a user's custom YAML may not).
        """
        available = self._blocks(sys_error)

        object_blocks_needed = {
            _PRIMARY_BLOCK_FOR_COLLECTION[c]
            for c in needed_collections
            if c in _OBJECT_COLLECTIONS and _PRIMARY_BLOCK_FOR_COLLECTION[c] in available
        }
        n_object_types_needed = len(object_blocks_needed)

        block_names = []
        for block_name in _CANONICAL_BLOCK_ORDER:
            if block_name == "Thinning":
                continue  # handled separately - it's a list, not a single block
            if block_name in ("CommonServices", "PileupReweighting"):
                if block_name in available:
                    block_names.append(block_name)
            elif block_name in object_blocks_needed:
                block_names.append(block_name)
            elif block_name == "MissingET":
                if "met_collection" in needed_collections and block_name in available:
                    block_names.append(block_name)
            elif block_name == "OverlapRemoval":
                if n_object_types_needed >= 2 and block_name in available:
                    block_names.append(block_name)
        return block_names

    def thinning_entries_for(
        self, accessed: FrozenSet[str], sys_error: str
    ) -> List[Tuple[str, Dict[str, Any]]]:
        """Return (collection_attr_name, thinning_entry) pairs for `Thinning`
        list entries whose container belongs to a collection in `accessed`.

        Deliberately scoped to `accessed` (what was *directly* asked for) and
        not the wider, dependency-expanded set `blocks_needed` uses - e.g.
        accessing only `.MissingET()` should not, by itself, cause electrons
        to be thinned for output; it only needs them calibrated as an input.
        """
        available = self._blocks(sys_error)
        thinning = available.get("Thinning", [])
        if isinstance(thinning, dict):
            thinning = [thinning]

        container_to_collection = {}
        for collection_attr_name, block_name in _PRIMARY_BLOCK_FOR_COLLECTION.items():
            block = available.get(block_name)
            if isinstance(block, dict) and "containerName" in block:
                container_to_collection[block["containerName"]] = collection_attr_name

        result = []
        for entry in thinning:
            owner = container_to_collection.get(entry.get("containerName"))
            if owner is not None and owner in accessed:
                result.append((owner, entry))
        return result

    def calibration_job_script_items(
        self, sys_error: str, accessed: FrozenSet[str]
    ) -> List[Dict[str, Any]]:
        """Build the `add_job_script` metadata items needed to configure this
        config's blocks for `accessed` (the full set of `collection_attr_name`s
        directly accessed so far in this query).

        Split one item per block/Thinning-entry rather than one big script:
        each item's content only depends on this config + sys_error, never on
        what else is in `accessed`, so re-sending the same one (e.g. `Jets`,
        because both a direct `.Jets()` call and `.MissingET()`'s dependency
        both need it) is always identical and safe to dedup. Only *which*
        items get sent varies with `accessed`; a `_finalize` item (fixed
        content, growing `depends_on`) assembles whatever arrived into one
        `TextConfig` in the right order.
        """
        needed_collections = _resolve_needed_collections(accessed)
        available = self._blocks(sys_error)

        var = f"_calib_blocks_{_sanitize_identifier(self.name)}_{_sanitize_identifier(sys_error)}"
        prefix = f"calibration_config_{self.name}_{sys_error}"
        init_name = f"{prefix}_init"
        finalize_name = f"{prefix}_finalize"

        items: List[Dict[str, Any]] = [
            {
                "metadata_type": "add_job_script",
                "name": init_name,
                "script": [f"{var} = {{}}"],
            }
        ]

        populated_names: List[str] = []

        def add_item(unique_part: str, script_lines: List[str]):
            item_name = f"{prefix}_block_{unique_part}"
            items.append(
                {
                    "metadata_type": "add_job_script",
                    "name": item_name,
                    "script": script_lines,
                    "depends_on": [init_name],
                }
            )
            populated_names.append(item_name)

        for block_name in self.blocks_needed(needed_collections, sys_error):
            add_item(
                block_name,
                [f"{var}[{block_name!r}] = {available[block_name]!r}"],
            )

        for collection_attr_name, entry in self.thinning_entries_for(
            accessed, sys_error
        ):
            add_item(
                f"Thinning_{collection_attr_name}",
                [f"{var}.setdefault('Thinning', []).append({entry!r})"],
            )

        items.append(
            {
                "metadata_type": "add_job_script",
                "name": finalize_name,
                "script": [
                    "from AnalysisAlgorithmsConfig.ConfigText import TextConfig",
                    (
                        f"config = TextConfig(config={{k: {var}[k] "
                        f"for k in {_CANONICAL_BLOCK_ORDER!r} if k in {var}}})"
                    ),
                ],
                "depends_on": [init_name] + populated_names,
            }
        )

        return items


def _load_calibration_config(name: str, path: Path) -> CalibrationConfig:
    return CalibrationConfig(name, path.read_text())


_g_default_config_paths = {
    "PHYS": Path(__file__).parent / "config_phys.yaml",
    "PHYSLITE": Path(__file__).parent / "config_physlite.yaml",
}


def default_calibration_name() -> str:
    "The default calibration config name used when nothing else is specified."
    return "PHYS"


class calib_tools:
    """Helper functions to work with a query's calibration configuration."""

    # Maps calibration name (e.g. "PHYS") to the path of the yaml file that
    # should be loaded for it. Starts out as the bundled defaults; overridden
    # via `set_calibration_yaml_path`.
    _calibration_yaml_paths: Optional[Dict[str, Path]] = None

    _default_sys_error: Optional[str] = "NOSYS"

    @classmethod
    def _setup(cls):
        if cls._calibration_yaml_paths is None:
            cls.reset_config()

    @classmethod
    def reset_config(cls):
        "Reset calibration config paths back to the bundled per-release defaults."
        cls._calibration_yaml_paths = dict(_g_default_config_paths)

    @classmethod
    def set_calibration_yaml_path(
        cls, path: str, config_name: Optional[str] = None
    ):
        """Point calibration config `config_name` (or the default one, if not
        given) at a user-supplied YAML file. All queries built after this call
        will use it in place of the bundled default.

        Args:
            path (str): Path to a YAML file, structured like `config_phys.yaml`
                (top-level keys are `TextConfig` block names handed through
                unmodified, plus an optional `_servicex_meta` key holding the
                front-end-only `calibrate_by_default`/`uncalibrated_possible`/
                `raw_collection_names` settings).
            config_name (Optional[str]): Which calibration config to replace
                (e.g. "PHYS" or "PHYSLITE"). Defaults to the default config name.
        """
        cls._setup()
        if config_name is None:
            config_name = default_calibration_name()

        assert cls._calibration_yaml_paths is not None
        cls._calibration_yaml_paths[config_name] = Path(path)

    @classmethod
    def reset_calibration_yaml_path(cls, config_name: Optional[str] = None):
        "Reset calibration config `config_name` back to its bundled default."
        cls._setup()
        if config_name is None:
            config_name = default_calibration_name()

        assert cls._calibration_yaml_paths is not None
        if config_name in _g_default_config_paths:
            cls._calibration_yaml_paths[config_name] = _g_default_config_paths[
                config_name
            ]
        else:
            del cls._calibration_yaml_paths[config_name]

    @classmethod
    def default_config(cls, config_name: Optional[str] = None) -> CalibrationConfig:
        """Return the calibration config for `config_name` (or the default one)."""
        cls._setup()
        if config_name is None:
            config_name = default_calibration_name()

        assert cls._calibration_yaml_paths is not None
        path = cls._calibration_yaml_paths[config_name]
        return _load_calibration_config(config_name, path)

    @classmethod
    def query_update(
        cls,
        query: ObjectStream[T],
        calib_config: Optional[CalibrationConfig] = None,
    ) -> ObjectStream[T]:
        """Add metadata to a query to indicate a change in the calibration
        configuration for the query.

        Args:
            query (ObjectStream[T]): The query to update.
            calib_config (Optional[CalibrationConfig]): The new calibration
                configuration to use. Defaults to the current default config.

        Returns:
            ObjectStream[T]: The updated query.
        """
        config = calib_config if calib_config is not None else cls.default_config()
        return query.QMetaData({"calibration": config})

    @classmethod
    def query_get(cls, query: ObjectStream[T]) -> CalibrationConfig:
        """Return the calibration config that would be in effect if the query
        were issued at this point.
        """
        assert query is not None, "Call to `query_get`: `query` argument is null."
        r = lookup_query_metadata(query, "calibration")
        if r is None:
            logging.warning(
                "Fetched the default calibration configuration for a query. It should "
                "have been intentionally configured - using configuration for data "
                f"format {default_calibration_name()}"
            )
            return calib_tools.default_config()
        return r

    @classmethod
    def default_sys_error(cls) -> str:
        "Return the default systematic error"
        if cls._default_sys_error is None:
            return "NOSYS"
        return cls._default_sys_error

    @classmethod
    def set_default_sys_error(cls, value: str):
        "Set the default systematic error"
        cls._default_sys_error = value

    @classmethod
    def reset_sys_error(cls):
        "Reset to 'NOSYS' the default systematic error"
        cls._default_sys_error = "NOSYS"

    @classmethod
    def query_sys_error(cls, query: ObjectStream[T], sys_error: str) -> ObjectStream[T]:
        """Add metadata to a query to indicate a change in the systematic error
        for the events.
        """
        return query.QMetaData({"calibration_sys_error": sys_error})


def fixup_collection_call(
    s: ObjectStream[T], a: ast.Call, collection_attr_name: str
) -> Tuple[ObjectStream[T], ast.Call]:
    "Apply all the fixes to the collection call"

    bank_name = None
    calibrate = None

    if len(a.args) >= 1:
        bank_name = ast.literal_eval(a.args[0])

    if len(a.args) >= 2:
        calibrate = ast.literal_eval(a.args[1])

    for arg in a.keywords:
        if arg.arg == "collection":
            bank_name = ast.literal_eval(arg.value)
        elif arg.arg == "calibrate":
            calibrate = ast.literal_eval(arg.value)
        else:
            raise TypeError(f'Unknown argument "{arg.arg}" to collection call.')

    new_s = s

    sys_error = lookup_query_metadata(new_s, "calibration_sys_error")
    if sys_error is None:
        sys_error = calib_tools.default_sys_error()

    config = calib_tools.query_get(new_s)

    if bank_name is None:
        # No override - use the config's own raw (pre-calibration) container
        # name for this type.
        bank_name = config.raw_collection_name(collection_attr_name)

    uncalibrated_possible = config.uncalibrated_possible()
    if calibrate is None:
        if sys_error != "NOSYS":
            calibrate = True
        else:
            calibrate = config.calibrate_by_default()
    else:
        if (not calibrate) and (not uncalibrated_possible):
            raise NotImplementedError(
                f"Requested uncalibrated {bank_name}, but that "
                "is not possible on this dataset type"
            )
    if sys_error != "NOSYS" and not calibrate:
        raise NotImplementedError(
            "Cannot request a systematic error and not have calibration run "
            f"for {bank_name}"
        )

    if not calibrate:
        output_collection_name = bank_name
    else:
        # Track every collection type directly accessed so far in this query
        # (scoped to this config + sys_error, since a query could in principle
        # mix systematics), so later accesses only add the extra blocks they
        # need instead of resending everything already sent.
        accessed_key = f"calibration_collections_used_{config.name}_{sys_error}"
        accessed_so_far = lookup_query_metadata(new_s, accessed_key) or frozenset()
        accessed_so_far = frozenset(accessed_so_far) | {collection_attr_name}
        new_s = new_s.QMetaData({accessed_key: accessed_so_far})

        for item in config.calibration_job_script_items(sys_error, accessed_so_far):
            new_s = new_s.MetaData(item)

        finalize_name = f"calibration_config_{config.name}_{sys_error}_finalize"
        wiring_text = _WIRING_TEMPLATE_PATH.read_text()
        new_s = new_s.MetaData(
            {
                "metadata_type": "add_job_script",
                "name": _WIRING_TEMPLATE_NAME,
                "script": wiring_text.splitlines(),
                "depends_on": [finalize_name],
            }
        )

        output_collection_name = config.output_collection_name(
            collection_attr_name, sys_error
        )

    new_call = copy.copy(a)
    new_call.args = [
        ast.parse(f"'{output_collection_name}'").body[0].value
    ]  # type: ignore

    return new_s, new_call
