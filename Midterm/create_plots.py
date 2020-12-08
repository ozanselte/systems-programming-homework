import re
from random import shuffle
from subprocess import check_output
import matplotlib.pyplot as plt

def test_parameters(N, M, T, S, L):
    arr = []
    students = []
    counter = []
    kitchen = []
    for i in range(L*M):
        arr.append('P')
        arr.append('C')
        arr.append('D')
    shuffle(arr)
    buf = ''.join(arr)
    f = open('inputX', 'w+')
    f.write(buf)
    f.close()
    out = check_output(
        "./midterm -N {} -M {} -T {} -S {} -L {} -F inputX".format(
            N, M, T, S, L
        ),
        shell=True
    ).decode('utf-8').split('\n')
    for line in out:
        if '# of students' in line:
            students.append(int(re.findall(r'\d+', line)[2]))
        if 'counter items' in line:
            counter.append(int(re.findall(r'\d+', line)[-1]))
        if 'kitchen items' in line:
            kitchen.append(int(re.findall(r'\d+', line)[-1]))
    return ([students, counter, kitchen])


defN = 6
defM = 24
defT = 10
defS = 8
defL = 26

results = []
for N in range(5, 21, 5):
    res = test_parameters(N, defM, defT, defS, defL)
    results.append([N, res])
plt.clf()
plt.xlabel('Time')
plt.ylabel('Students at the Counter')
for result in results:
    plt.plot(result[1][0], label='N='+str(result[0]))
plt.legend()
plt.savefig('./figs/N_students.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Counter')
for result in results:
    plt.plot(result[1][1], label='N='+str(result[0]))
plt.legend()
plt.savefig('./figs/N_counter.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Kitchen')
for result in results:
    plt.plot(result[1][2], label='N='+str(result[0]))
plt.legend()
plt.savefig('./figs/N_kitchen.png')

results = []
for M in range(15, 61, 15):
    res = test_parameters(defN, M, defT, defS, defL)
    results.append([M, res])
plt.clf()
plt.xlabel('Time')
plt.ylabel('Students at the Counter')
for result in results:
    plt.plot(result[1][0], label='M='+str(result[0]))
plt.legend()
plt.savefig('./figs/M_students.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Counter')
for result in results:
    plt.plot(result[1][1], label='M='+str(result[0]))
plt.legend()
plt.savefig('./figs/M_counter.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Kitchen')
for result in results:
    plt.plot(result[1][2], label='M='+str(result[0]))
plt.legend()
plt.savefig('./figs/M_kitchen.png')

results = []
for T in range(5, 21, 5):
    res = test_parameters(defN, defM, T, defS, defL)
    results.append([T, res])
plt.clf()
plt.xlabel('Time')
plt.ylabel('Students at the Counter')
for result in results:
    plt.plot(result[1][0], label='T='+str(result[0]))
plt.legend()
plt.savefig('./figs/T_students.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Counter')
for result in results:
    plt.plot(result[1][1], label='T='+str(result[0]))
plt.legend()
plt.savefig('./figs/T_counter.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Kitchen')
for result in results:
    plt.plot(result[1][2], label='T='+str(result[0]))
plt.legend()
plt.savefig('./figs/T_kitchen.png')

results = []
for S in range(15, 61, 15):
    res = test_parameters(defN, defM, defT, S, defL)
    results.append([S, res])
plt.clf()
plt.xlabel('Time')
plt.ylabel('Students at the Counter')
for result in results:
    plt.plot(result[1][0], label='S='+str(result[0]))
plt.legend()
plt.savefig('./figs/S_students.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Counter')
for result in results:
    plt.plot(result[1][1], label='S='+str(result[0]))
plt.legend()
plt.savefig('./figs/S_counter.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Kitchen')
for result in results:
    plt.plot(result[1][2], label='S='+str(result[0]))
plt.legend()
plt.savefig('./figs/S_kitchen.png')

results = []
for L in range(15, 61, 15):
    res = test_parameters(defN, defM, defT, defS, L)
    results.append([L, res])
plt.clf()
plt.xlabel('Time')
plt.ylabel('Students at the Counter')
for result in results:
    plt.plot(result[1][0], label='L='+str(result[0]))
plt.legend()
plt.savefig('./figs/L_students.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Counter')
for result in results:
    plt.plot(result[1][1], label='L='+str(result[0]))
plt.legend()
plt.savefig('./figs/L_counter.png')
plt.clf()
plt.xlabel('Time')
plt.ylabel('Plates at the Kitchen')
for result in results:
    plt.plot(result[1][2], label='L='+str(result[0]))
plt.legend()
plt.savefig('./figs/L_kitchen.png')