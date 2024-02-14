#!/usr/bin/python 

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost,RemoteController
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel

from mininet.node import Controller
import os


class Mytopo(Topo):
    def build(self):
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')
        s3 = self.addSwitch('s3')

        h1 = self.addHost('h1')
        h2 = self.addHost('h2')

        h3 = self.addHost('h3')
        h4 = self.addHost('h4')
        h5 = self.addHost('h5')

        h6 = self.addHost('h6')

        self.addLink(s1, h1,bw=10)
        self.addLink(s1, h2)

        self.addLink(s2, h3)
        self.addLink(s2, h4)
        self.addLink(s2, h2)

        self.addLink(s3, h2)
        self.addLink(s3, h5)
        self.addLink(s3, h6)

        self.addLink(s1, s2)
        self.addLink(s2,s3)

if __name__ == '__main__':
	setLogLevel( 'info' )
	net = Mininet( topo=Mytopo(),link=TCLink,host=CPULimitedHost, controller=lambda name: RemoteController( name, ip='127.0.0.1' ) )
	net.start()
	print( "Dumping host connections" )
	dumpNodeConnections( net.hosts )
	print( "Testing network connectivity" )
	net.pingAll()
	print( "Testing bandwidth between h1 and h6" )
	h1, h6 = net.get( 'h1', 'h6' )
	h1.cmdPrint('tcpdump -i h1-eth0 -w /tmp/h1_capture.pcap &')
	net.iperf( (h1, h6) )
	net.stop()
