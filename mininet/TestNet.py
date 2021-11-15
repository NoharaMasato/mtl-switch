from mininet.net import Mininet
from mininet.topo import Topo
from mininet.cli import CLI
from mininet.log import setLogLevel
from mininet.node import Switch, Host, NullController
import os


HOST_NUM=30
BASE_PATH = os.path.join(os.path.dirname(__file__), "..")
SWITCH_EXEC_FILE = os.path.join(BASE_PATH, "src/mtl-switch")
LOG_DIR = os.path.join(BASE_PATH, "mininet/log/")


class HostV4(Host):
    def __init__(self, *args, **kwargs):
        super( HostV4, self ).__init__(*args, **kwargs)
        cfgs = [ 'all.disable_ipv6=1', 'default.disable_ipv6=1',
                 'default.autoconf=0', 'lo.autoconf=0' ]
        for cfg in cfgs:
            self.cmd( 'sysctl -w net.ipv6.conf.' + cfg )


class MTLSwitch(Switch):
    def __init__(self, name, **kwargs):
        Switch.__init__(self, name, **kwargs)
        cfgs = [ 'all.disable_ipv6=1', 'default.disable_ipv6=1',
                 'default.autoconf=0', 'lo.autoconf=0' ]
        for cfg in cfgs:
            self.cmd( 'sysctl -w net.ipv6.conf.' + cfg )

    def start(self, _controllers):
        print("[My Bridge] bridge start")

        # set mtu
        for intf_id in self.intfs:
            intf_name = self.intfs[intf_id].name
            self.cmd(f"ifconfig {intf_name} mtu 9000")

        self.cmd(SWITCH_EXEC_FILE, self.name, ">>", LOG_DIR + self.name + ".log", "2>>", LOG_DIR + self.name + "-error.log", "&")


class MyTopo(Topo):
    "Custom topology example."

    def build( self ):
        "Build custom topo."

        switch = self.addSwitch('s1', cls=MTLSwitch)
        for i in range(HOST_NUM):
            h = self.addHost(f"h{i + 1}", ip=f"10.0.0.{i + 1}", mac='00:00:00:00:00:' + format(i + 1, '02x'), cls=HostV4)
            self.addLink(h, switch)


if __name__ == '__main__':
    setLogLevel( 'info' )
    net = Mininet(topo=MyTopo(), controller=NullController)
    net.start()

    CLI(net)

    net.stop()
