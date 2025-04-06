# Manual testing

To manually test the node and it's parsing library while developing. There are some small convenient scripts which make a virtual serial port, to which the library can be hooked up. This provides an easy way to validate the TLV parsing, without having to hookup the Radar EVM to your pc.

**This guide is Linux only!**

## What to do first?
Create a virtual serial port by running:
```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```
The output looks something like this:
```bash
$ socat -d -d pty,raw,echo=0 pty,raw,echo=0
2025/04/06 21:40:15 socat[130785] N PTY is /dev/pts/6
2025/04/06 21:40:15 socat[130785] N PTY is /dev/pts/13
2025/04/06 21:40:15 socat[130785] N starting data transfer loop with FDs [5,5] and [7,7]
```
Keep an eye out for the pts/6 and pts/13, you will need them later. As these are two virtual ports which are linked to each other, meaning one-end will be connected to the app. And one to your shell script.

**The process is blocking, thus you need to keep socat running on the background.**

**The radar has two serial ports, instantiate this socat thing in two seperate terminals.** 
```bash
$ socat -d -d pty,raw,echo=0 pty,raw,echo=0
2025/04/06 21:45:11 socat[132988] N PTY is /dev/pts/2
2025/04/06 21:45:11 socat[132988] N PTY is /dev/pts/14
2025/04/06 21:45:11 socat[132988] N starting data transfer loop with FDs [5,5] and [7,7]
```

Okay you're almost set; Now adjust the ports in the script:

```
CMD_WRITE="/dev/pts/13"  
DATA_WRITE="/dev/pts/14"   
```

## How to use the mock_radar.sh script

Well simple after you set above section up for your machine. Just run:
```bash
sh ./mock_radar.sh
```
Which will say something like this:
```bash
$ sh ./mock_radar.sh 
[Listening on /dev/pts/13 for 'sensorStart']
```
Great! Now run the program node with the parameters:
