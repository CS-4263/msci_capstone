# Deployment Instructions

1. create a google cloud vm.
2. enable HTTP traffic on the vm.
3. compile the HTTPEcho App.
4. sudo run it and it will passively capture HTTP traffic going through the vm.  

It will then save the captured traffic in a separate directory to be used for replay at a later time.  

Optional 5. HTTPEcho can handle pcap files as well, in case the capture is already saved to a pcap file.
