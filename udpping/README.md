udpping
=======

A simple udp echo client/server with configureable payload size

Manly written for embedded devices with no non volatile memory to spare.

# NAT
In client mode this utility can be placed behind a nat firewall.
It listens on the source port used for outgoing packets, thus the NAT should redirect all responses to the correct port.
