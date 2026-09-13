from typing import Dict

from .calibration_event_config import CalibrationEventConfig


def default_calibration_name() -> str:
    """Should return the default configuration name from the calibration
    dictionary that should be used if the user doesn't specify anything else.

    Returns:
        str: _description_
    """
    return "PHYS"


def default_calibration_config() -> Dict[str, CalibrationEventConfig]:
    """Return the default calibration configuration for this release
    (this is based on R21 here). We return a config for each type of
    data typically encountered in the experiment.

    The user can change the default config - this is only used when nothing
    else is provided (or the default is explicitly requested). In general, a
    user shouldn't call this, but use the `calib_tools` object.

    This is in its own file to enable changes and updates for different
    releases to be easily handled.

    PHYS - for working with R21 DAOD_PHYS formats

    Returns:
        Dict[str, CalibrationEventConfig]: The default config.
    """
    return {
        "PHYS": CalibrationEventConfig(
            jet_collection="AntiKt4EMPFlowJets",
            jet_calib_truth_collection="AntiKt4TruthDressedWZJets",
            run_jet_ghost_muon_association=True,
            electron_collection="Electrons",
            electron_working_point="MediumLHElectron",
            electron_isolation="NonIso",
            photon_collection="Photons",
            photon_working_point="Tight",
            photon_isolation="FixedCutTight",
            muon_collection="Muons",
            muon_working_point="Medium",
            muon_isolation="NonIso",
            tau_collection="TauJets",
            tau_working_point="Tight",
            met_collection="MissingET",
            perform_overlap_removal=True,
            datatype="mc",
            calibrate=True,
            uncalibrated_possible=True,
            correct_pileup=True,
            fix_daod_truth_record=True,
        )
    }
