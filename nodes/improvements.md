# In general

- Handle a failed `openCommunication()` in `setup()`.

# Node1

- Add timeout in run(). If scale stops responding or simularly, run should return some error. Diffucalty: Different pot sizes diffrent timeouts.

- The QR scan is blocking: while get_qrcode() waits, an abort can't be handled.

# Need to be fixed

loop and setup need to be removed / renamed in node.cpp. setup is importnat and need to be used some way in node codes. Small changes made inside test_node that needs to be exported to the res of the node codes. (node.h and node.cpp and senddata and sendButton)
