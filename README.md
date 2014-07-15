rtorrent
========

Exectution Parameters: 
`<exe>` `<s/c>` `<PortNumber>`

A bit-torrent style distributed file transfer client. A file requested at any client is fetched chunk-wise from all connected clients over TCP connections.

USAGE:

HELP : Open this page

MYIP :Display the IP address of this host.

MYPORT : Display the port on which this process is listening for incoming connections.

REGISTER `<server IP>` `<port_no>`: Used to register  with the server and to get the IP and listening port numbers of all the peers currently registered with the server.

SIP : Displays details of the other hosts that you can connect to.

CONNECT `<destination>` `<port no>`: Establishes a new TCP connection to the
specified `<destination>` at the specified `<port no>`.

LIST: Display a numbered list of all the connections this process is part of.

TERMINATE `<connection id>` This command will terminate the connection listed under the specified number when LIST is used to display all connections.

EXIT Close all connections and terminate this process.

DOWNLOAD `<file_name>` `<file_chunk_size_in_bytes>` : The host shall download in parallel different chunks of file specified in the download command from all connected hosts until the complete file is downloaded.
