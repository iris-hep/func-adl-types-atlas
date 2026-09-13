from AnalysisAlgorithmsConfig.ConfigText import TextConfig

config = TextConfig()

config.addBlock('CommonServices')
# Only run systematics if sys_error is not NOSYS
config.setOptions(runSystematics={{ 'False' if sys_error == 'NOSYS' else 'True' }})
config.setOptions(filterSystematics="^(?=.*{{sys_error}}|$).*")

import logging
logging.basicConfig(level=logging.INFO)