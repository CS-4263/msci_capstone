# Testing Process
Our primary mode of testing our project is by using live traffic capture. We do this by having our project on a GCP VM with an external IP set up. Then we run a simple HTTP server on the VM and access our directory via the external IP. Then we run our project which has the ability to live capture traffic and we filter it to port 80 in order to only get the traffic we are creating. Then while the project is running, we click on the directory links which send HTTP Requests to our VM. Our program does a live capture of this traffic and prints out the request and response. We can see any errors that occur by examining the output.

# Pass/Fail
In order for us to consider the program as "passing" the test is if it correctly displays all fields of both the request and response as well as the body for both. If it is missing any of these fields then we consider it "failing".

# Future testing
In the future we would need to test for the replay side of things which would require us to test that the program is correctly creating the packets and sending them to the server as requests.
