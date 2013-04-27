f = open('RoR.log', 'r')
lines=f.readlines()
f.close()

locklist = {}
locklisttrack = {}
counter = 0
nl = []
for line in lines:
	counter+=1
	if line.find('***MUTEX') == -1:
		continue
		
	mutexnum = int(line.split(':')[4].strip().split(' ')[1])

	unlock   = line.find('***MUTEX-UNLOCK')
	lock     = line.find('***MUTEX-LOCK')
	existing = mutexnum in locklist.keys()

	if lock != -1 and not existing:
		locklist[mutexnum] = 1
		locklisttrack[mutexnum] = counter
		continue
	if unlock != -1 and not existing:
		print "ERROR: tried to unlock non locked mutex, line ", counter
		continue
	
	if lock != -1 and locklist[mutexnum] != 1:
		locklist[mutexnum] = 1
		locklisttrack[mutexnum] = counter
	elif lock != -1 and locklist[mutexnum] == 1:
		print "DEADLOCK       with mutex ", mutexnum, " in line ", counter, " previously locked in line ", locklisttrack[mutexnum]

	elif unlock != -1 and locklist[mutexnum] != 0:
		locklist[mutexnum] = 0
		locklisttrack[mutexnum] = counter
	elif unlock != -1 and locklist[mutexnum] == 0:
		print "DOUBLE UNLOCK  with mutex ", mutexnum, " in line ", counter
	
