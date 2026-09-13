configSeq = config.configure()

from Campaigns.Utils import Campaign

# Needed to configure the AlgSequence
from AthenaConfiguration.AllConfigFlags import initConfigFlags

flags = initConfigFlags()
flags.Input.Files = [sh.at(0).fileName(0)]
flags.lock()

autoconfigFromFlags = flags

logging.info("Adding Calibration")

from AnaAlgorithm.AlgSequence import AlgSequence
algSeq = AlgSequence()

from AnalysisAlgorithmsConfig.ConfigAccumulator import ConfigAccumulator
configAccumulator = ConfigAccumulator(algSeq=algSeq, autoconfigFromFlags=autoconfigFromFlags)
configSeq.fullConfigure(configAccumulator)

algSeq.addSelfToJob( job )
print(job) # for debugging