#!/bin/bash

trap failwhale SIGINT

failwhale() {
echo "FAIL WHALE"
exit
}

# pipexec -k (kills other processes if one dies)
# ^ This fixes problem when you hangup on netcat (MEOOOW)

while true
do
  pipexec -k -- [ NETCAT /bin/netcat -l 2023 ] [ DOOR build/space-ace -D DOOR.SYS ] "{NETCAT:1>DOOR:0}" "{DOOR:1>NETCAT:0}"
  echo "Let's do it again!"

done

