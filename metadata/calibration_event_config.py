from dataclasses import dataclass


@dataclass
class CalibrationEventConfig:
    # Name of the jet collection to calibrate and use by default
    jet_collection: str

    # Name of the truth jets to be used for the jet calibration
    jet_calib_truth_collection: str

    # SHoudl the muon ghost association run? No if this is phys lite
    run_jet_ghost_muon_association: bool

    # ** Electrons
    # Name of the electron collection to calibrate and use by default
    electron_collection: str

    # The working point (e.g. xxx)
    electron_working_point: str

    # The isolation (e.g. xxxx)
    electron_isolation: str

    # ** Photons
    # Name of the photon collection to calibrate and use by default.
    photon_collection: str

    # The working point (e.g. xxx)
    photon_working_point: str

    # The isolation (e.g. xxxx)
    photon_isolation: str

    # ** Muons
    # Name of the muon collection to calibration and use by default.
    muon_collection: str

    # The working point (e.g. xxx)
    muon_working_point: str

    # The isolation (e.g. xxxx)
    muon_isolation: str

    # ** Taus
    # Name of the tau collection to calibrate and use by default.
    tau_collection: str

    # The working point (e.g. xxxx)
    tau_working_point: str

    # ** MET
    # The name of the MET collection to calibrate and use by default.
    met_collection: str

    # ** Other Config Options
    perform_overlap_removal: bool

    # ** Data Type (data, MC, etc., used for pileup, jet corrections, etc.)
    datatype: str

    # Perform pileup correction. Almost should do it.
    correct_pileup: bool

    # ** Run calibrations by default (PHYSLITE vs PHYS)
    calibrate: bool

    # ** True if we can return uncalibrated (PHYSLITE doesn't)
    uncalibrated_possible: bool

    # ** Whether to fix up older-style DAOD truth records
    fix_daod_truth_record: bool
