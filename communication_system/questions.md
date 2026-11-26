# Questions on node code

line 144: Why random wait?

line 176: why do I need the node id?

(espnowprotocol) line 25: What is Node Assignment?

line 157: How does `requestCommand` and `requestParams` look like? Is it defined in `espNowProtocol`? I do not follow the `processRequest` function.


Overall node flow:
How and which function shuld be used to do what (in `node.h`). How does a `userRequestHandler` look like, and how does the `requestCommands` look like. 
Have I under stod right if I think that the `requestCommand` is the command telling the node what it should do. Like *take water measurement*.
If so what is `requestParams` then? Is it like, *water: 50ml*? 
