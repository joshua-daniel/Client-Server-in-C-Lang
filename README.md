"# Client-Server-in-C-Lang" 

This is project is done in Debian Linux, so some system calls may not work in other Operating Systems

-- in server machine

$ gcc -o server server.c
$ ./server <port-number>

-- in different terminal
$ gcc -o mirror1 mirror1.c         
$./mirror1 9191 

-- in different terminal
$ gcc -o mirror2 mirror2.c         
$./mirror2 9192


-- in client machine

$ gcc -o client client.c
$ ./client <server-ip-address> <port-number> 


$ hostname -i 
Execute above command in Server machine , you will get IP address of server . Use it in client machine . 
<port-number> should be same in server and client.
