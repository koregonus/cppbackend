import argparse
import subprocess
import time
import random
import shlex
import sys

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None, input=None):
    process = subprocess.Popen(shlex.split(command), stdin=input, stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


server = run(start_server())

params = 'perf record -g -p ' + str(server.pid) + ' -o perf.data'
# print(params)
perf = run(params)
make_shots()

stop(perf)
stop(server)
time.sleep(1)
pipe1 = run('perf script -i perf.data', subprocess.PIPE)
pipe2 = run('./FlameGraph/stackcollapse-perf.pl', subprocess.PIPE, pipe1.stdout)
f = open("graph.svg", "w")
pipe3 = run('./FlameGraph/flamegraph.pl', f, pipe2.stdout)
f.close()
time.sleep(3)
f = open("graph.svg", "r")

if 'RequestHandler' in f.read():
    print('ReqFound')
else:
    for line in f.readlines():
        print(line)
f.close()


print('Job done')
