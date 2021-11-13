from mininet.topo import Topo
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.node import NullController

from MyClass import MyBridge, HostV4, MyNetwork

import time

HOST_NUM=80

class MyTopo( Topo ):
    "Custom topology example."

    def build( self ):
        "Build custom topo."

        switch = self.addSwitch('s1', cls=MyBridge)
        for i in range(HOST_NUM):
            h = self.addHost(f"h{i + 1}", ip=f"10.0.0.{i + 1}", mac='00:00:00:00:00:' + format(i + 1, '02x'), cls=HostV4)
            self.addLink(h, switch)

if __name__ == '__main__':
    setLogLevel( 'info' )
    net = MyNetwork(topo=MyTopo(), controller=NullController)
    net.start()

    # net.ping_test()
    CLI(net)

    net.stop()
