# sudo apt-get install vpp-dev 
# ls /usr/share/vpp/api
# cat l2.api.json
# cat interface.api.json

gcc -g -O0 vppclient.c -o vppclient -lvppinfra -lvlibmemoryclient -lsvm

gcc -g -O0 -o vapi_client -lvppinfra -lvlibmemoryclient -lsvm -lvapiclient
