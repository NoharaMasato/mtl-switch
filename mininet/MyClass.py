from mininet.node import Switch, Host
from mininet.nodelib import LinuxBridge
from mininet.net import Mininet

import time
import os

BASE_PATH = os.path.join(os.path.dirname(__file__), "..")

SWITCH_EXEC_FILE = os.path.join(BASE_PATH, "switch/FMSwitch")
CONTROLLER_EXEC_FILE = os.path.join(BASE_PATH, "controller/controller")
LOG_DIR = os.path.join(BASE_PATH, "mininet/log/")

REDIS_INIT_SWITCH_FILE = os.path.join(BASE_PATH, 'redis/insert_tag_table.sh')
REDIS_INIT_CONTROLLER_FILE = os.path.join(BASE_PATH, 'redis/insert_controller_acl.sh')
REDIS_TAG_TABLE_FILE = os.path.join(BASE_PATH, 'redis/tag_table_vcrm.txt')
REDIS_CONF_FILE = os.path.join(BASE_PATH, 'redis/redis.conf')
REDIS_MONITOR_SCRIPT = os.path.join(BASE_PATH, 'redis/monitor_redis.py')

CONTROLLER_IP="10.0.100.100"


class MyBridge(Switch):
    def __init__(self, name, **kwargs):
        Switch.__init__(self, name, **kwargs)
        cfgs = [ 'all.disable_ipv6=1', 'default.disable_ipv6=1',
                 'default.autoconf=0', 'lo.autoconf=0' ]
        for cfg in cfgs:
            self.cmd( 'sysctl -w net.ipv6.conf.' + cfg )

    def start(self, _controllers):
        print("[My Bridge] bridge start")

        # redisがnamespaceに入っている場合のみ、それぞれのswitchでredisを起動
        if self.inNamespace:
            self.cmd("ip", "addr", "add", "127.0.0.1", "dev", "lo")
            self.cmd("ip", "link", "set", "dev", "lo", "up")
            self.cmd("/usr/bin/redis-server", REDIS_CONF_FILE, "&")
            self.cmd(REDIS_INIT_SWITCH_FILE, REDIS_TAG_TABLE_FILE, self.name, "&")

            self.cmd(REDIS_MONITOR_SCRIPT , ">>", LOG_DIR + self.name + "-redis.log", "2>>", LOG_DIR + self.name + "-redis-error.log", "&")

        # mtuを設定する
        for intf_id in self.intfs:
            intf_name = self.intfs[intf_id].name
            self.cmd(f"ifconfig {intf_name} mtu 9000")

        # return # コメントインすると、switchが立ち上がらなくなり、コマンドで立ち上げる必要がある（デバッグ用）

        # コントローラが立ち上がるまでsleepする
        self.cmd("sleep 3 &&", SWITCH_EXEC_FILE, self.name, ">>", LOG_DIR + self.name + ".log", "2>>", LOG_DIR + self.name + "-error.log", "&")


class HostV4(Host):
    def __init__(self, *args, **kwargs):
        super( HostV4, self ).__init__(*args, **kwargs)
        cfgs = [ 'all.disable_ipv6=1', 'default.disable_ipv6=1',
                 'default.autoconf=0', 'lo.autoconf=0' ]
        for cfg in cfgs:
            self.cmd( 'sysctl -w net.ipv6.conf.' + cfg )


# mininet内に存在するコントローラ
class MyController(Host):
    def __init__(self, *args, **kwargs):
        kwargs["ip"] = CONTROLLER_IP
        super(MyController, self).__init__(*args, **kwargs)
        cfgs = [ 'all.disable_ipv6=1', 'default.disable_ipv6=1',
                 'default.autoconf=0', 'lo.autoconf=0' ]
        for cfg in cfgs:
            self.cmd( 'sysctl -w net.ipv6.conf.' + cfg )

        # redisを起動
        self.cmd("/usr/bin/redis-server", REDIS_CONF_FILE, "&")
        self.cmd("sleep 3 && ", REDIS_INIT_CONTROLLER_FILE, "&") # sleepしたらredisに反映されるようになった
        # hostがmininetで作成される時間くらいsleepすれば良さそう

        # controllerを起動
        self.cmd(CONTROLLER_EXEC_FILE, ">>", LOG_DIR + "c1.log", "2>>", LOG_DIR + "c1-error.log", "&")


# switchをnamespaceに入れるときも入れないときも使用可能
class MyNetwork(Mininet):
    def configureControlNetwork(self):
        "Configure control network."
        pass

    def ping_test(self, num=5):
        time.sleep(3)
        start = time.time()
        for i in range(num):
            result = self.ping()
            if result != 0:
                print("Test Fail")
                break
        elapsed_time = time.time() - start
        print ("elapsed_time:{0}".format(elapsed_time) + "[sec]")


class LinuxBridgeV4(LinuxBridge):
    def __init__(self, *args, **kwargs):
        super( LinuxBridgeV4, self ).__init__(*args, **kwargs)
        cfgs = [ 'all.disable_ipv6=1', 'default.disable_ipv6=1',
                 'default.autoconf=0', 'lo.autoconf=0' ]
        for cfg in cfgs:
            self.cmd( 'sysctl -w net.ipv6.conf.' + cfg )

        # switchをnamespaceに入れない時に、これをしないとlinuxbridgeは動かなくなる(mininet起動時の警告は消えないが)
        self.cmd('sysctl -w net.bridge.bridge-nf-call-arptables=0')
        self.cmd('sysctl -w net.bridge.bridge-nf-call-iptables=0')
        self.cmd('sysctl -w net.bridge.bridge-nf-call-ip6tables=0')
