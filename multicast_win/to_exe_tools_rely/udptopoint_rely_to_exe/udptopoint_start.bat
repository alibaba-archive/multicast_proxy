cd "C:\udptopoint\"

WinPcap.exe

sc create udptopoint binpath= "c:\udptopoint\udptopoint.exe" start= auto

sc start udptopoint