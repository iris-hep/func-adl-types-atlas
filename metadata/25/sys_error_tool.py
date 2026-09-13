from AnalysisAlgorithmsConfig.ConfigText import TextConfig

config = TextConfig()

config.addBlock('CommonServices')
# Only run systematics if sys_error is not NOSYS
config.setOptions(runSystematics={{ 'False' if sys_error == 'NOSYS' else 'True' }})
config.setOptions(filterSystematics="^(?=.*{{sys_error}}|$).*")
config.setOptions(fixDAODTruthRecord={{calib.fix_daod_truth_record}})

import logging
logging.basicConfig(level=logging.INFO)