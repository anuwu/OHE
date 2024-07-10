#!/bin/zsh
(cat /etc/pf.conf && echo "dummynet-anchor \"mop\"" && echo "anchor \"mop\"") | sudo pfctl -f -
echo "dummynet in quick proto tcp from any to any port 11111 pipe 1" | sudo pfctl -a mop -f -
sudo dnctl pipe 1 config bw $1Mbit/s
