from pyndn import Name
from pyndn import Data
from pyndn.security import KeyChain

import sys

from RepoSocketPublisher import RepoSocketPublisher

publisher = RepoSocketPublisher(12345)

keyChain = KeyChain()

max_packets = 50000

total_size = 0

n = Name("/test").append(str(0))
d = Data(n)
d.setContent("this is a test.")
d.getMetaInfo().setFreshnessPeriod(4000)
keyChain.sign(d, keyChain.getDefaultCertificateName())

for i in range(0, max_packets + 1):
    n = Name("/test").append(str(i))
    d.setName(n)
    if i % 100000 == 0:
        print str(i)
    if i == max_packets:
        print str(total_size)
    total_size = total_size + d.wireEncode().size()
    publisher.put(d)
