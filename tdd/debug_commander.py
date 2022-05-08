import socket
import asyncio
import argparse
#################################################################################
# Example-Code
#   - Server : $ python3.9 ./test_udp.py -portl 20001
#   - Client : $ python3.9 ./test_python/test_udp.py  -file ./One-Time_test.txt
#              $ python3.8 ./test_python/test_udp.py  -file ./One-Time_test.txt  -ipr 192.168.1.6  -portr 20000
#################################################################################

parser = argparse.ArgumentParser()
parser.add_argument('-file', type=str, dest="file", help=' : file-path to be sent to Server.', default=None) 
parser.add_argument('-ipr', type=str, dest="remote_ip", help=' : Server IP address.', default='192.168.1.6')
parser.add_argument('-portr', type=int, dest="remote_port", help=' : Server Port number.', default=20000)
parser.add_argument('-portl', type=int, dest="local_port", help=' : Server Port number.', default=None)
args = parser.parse_args()

LOCAL_PORT = None
MAX_MSG_BYTE_SIZE = 8192            # UDP have a Limitation that is following like;
                                    # At part of UDP-receiving, It can receive message once a request within Read-Buffer size.
                                    #    After receiving a message, remained message is discarded.
                                    #    So, Sender have to sync Max-Send-msg-size with Read-Buffer size of Receiver.
SEND_BUF_SIZE = MAX_MSG_BYTE_SIZE 
RECV_BUF_SIZE = MAX_MSG_BYTE_SIZE


def udp_runner(peer_addr: tuple, mesg: str):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM, proto=socket.IPPROTO_UDP) as socUDP:
            set_udp_property(socUDP)

            async def receiver():
                msg = None
                global RECV_BUF_SIZE
                nonlocal peer_addr
                while True:
                    print("Ready to receive message.")
                    msg, peer_addr = socUDP.recvfrom(RECV_BUF_SIZE)
                    msg = msg.decode('utf8')
                    print(f"New data: {msg} from {peer_addr}")
                    await asyncio.sleep(0.5)

            async def sender():
                nonlocal mesg
                nonlocal peer_addr
                while True:
                    if mesg is not None and peer_addr is not None:
                        print("Try to send message to Server.")
                        socUDP.sendto(mesg.encode(),peer_addr)
                        mesg = None
                    await asyncio.sleep(0.5)

            async def runner():
                # register coroutines to scheduler as tasks.
                await asyncio.gather(*[ sender(), receiver() ])
            # run coroutine[runner] base on asyncio-thread(1 thread).
            #     asyncio-thread run tasks in scheduler that is registered.
            asyncio.run( runner() )      # Condition: Over than Python-Version 3.7
            
    except BaseException as e:
        print(f"Error: {e}")
        raise e


def set_udp_property(sock):
    global LOCAL_PORT
    global SEND_BUF_SIZE
    try:
        if LOCAL_PORT is not None:
            print(f"Set Local-Port.({LOCAL_PORT})")
            sock.bind(('', LOCAL_PORT))
            
        # Get the size of the socket's send buffer 
        bufsize = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF) 
        print ("Buffer size [Before]:%d" %bufsize) 
        
        sock.setsockopt( socket.SOL_SOCKET, socket.SO_SNDBUF, SEND_BUF_SIZE)
        bufsize = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
        print ("Buffer size [After]:%d" %bufsize) 
        
    except BaseException as e:
        print(f"Error: {e}")
        raise e


def parse_args(args):
    try:
        msg = None
        peer_addr = None
        global LOCAL_PORT
        LOCAL_PORT = args.local_port
        global MAX_MSG_BYTE_SIZE
        
        # extract Peer Address
        if args.remote_ip is not None or args.remote_port is not None:
            peer_addr   = (args.remote_ip, args.remote_port)
        print(f"Peer-Addr : {peer_addr}")
        
        # extract Message
        if args.file is not None:
            with open(args.file, mode="r+") as fp:
                msg = fp.read().rstrip() # .replace('\n', '')
                if msg is None or len(msg) == 0:
                    raise RuntimeError(f"msg is not exist in file.({args.file})")
                if len(msg) >= MAX_MSG_BYTE_SIZE:
                    raise RuntimeError(f"Message Size({len(msg)}) is over than MAX_MSG_SIZE.({MAX_MSG_BYTE_SIZE})")
                
        return peer_addr, msg
    except BaseException as e:
        print(f"Errot: {e}")
        raise e


def main(args) : 
    print("Start of Program.")
    print("")

    try:
        peer_addr, msg = parse_args(args)
        udp_runner(peer_addr, msg)
    except BaseException as e:
        print(f"Error: {e}")
    
    print("")
    print("End of Program.")
    
if __name__ == '__main__' :
    main(args)
