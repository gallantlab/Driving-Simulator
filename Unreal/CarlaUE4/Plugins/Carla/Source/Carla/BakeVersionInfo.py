#!/usr/bin/python
import re
import subprocess
import socket
import codecs
from datetime import datetime

settingsCPPFile = open('./Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Settings/CarlaSettings.h', 'r')
lines = []
line = settingsCPPFile.readline()
while line != '':
	lines.append(line)
	line = settingsCPPFile.readline()
settingsCPPFile.close()

settingsCPPFile = open('./Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Settings/CarlaSettings.h', 'w')

git = subprocess.Popen('git log -1', shell = True, stdout = subprocess.PIPE)
status = git.stdout.read().splitlines()
commit = codecs.decode(status[-1][4:], 'ascii')
git = subprocess.Popen('git status', shell = True, stdout = subprocess.PIPE)
branch = codecs.decode(git.stdout.readline()[10:-1], 'ascii')
git = subprocess.Popen('git describe --tags', shell = True, stdout = subprocess.PIPE)
versionString = codecs.decode(git.stdout.read(), 'ascii')
versionMatch = re.match("[A-Za-z]*-[0-9]*", versionString)
if (versionMatch):
	version = versionMatch.group(0)
else:
	version = versionString[:-1]
now = datetime.now()

resetBuild = False

settingsCPPFile.seek(0)
for i in range(len(lines)):
	line = lines[i]
	if "FString CompilingMachine" in line:
		line = "FString CompilingMachine = FString(\"{}\");\n".format(socket.gethostname())
	elif "FString GitCommit" in line:
		line = "FString GitCommit = FString(\"{}\");\n".format(commit)
	elif "FString GitBranch" in line:
		line = "FString GitBranch = FString(\"{}\");\n".format(branch)
	elif "FString MajorVersion" in line:
		lastVersion = line[38:-4]
		if lastVersion != version:
			resetBuild = True
		line = "FString MajorVersion = FString(\"{}\");\n".format(version)
	elif "FString CompileDate" in line:
		line = "FString CompileDate = FString(\"{}\");\n".format(now.strftime('%A, %B %-d, %Y %H:%M'))
	elif "int BuildVersion" in line:
		newVersion = 1
		if not resetBuild:
			newVersion = int(line.split(';')[0].split()[-1]) + 1
		line = "const int BuildVersion = {};\n".format(newVersion)
	settingsCPPFile.write(line)
settingsCPPFile.close()