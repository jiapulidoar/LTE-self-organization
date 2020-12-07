#!/usr/bin/env python3
# -- coding: utf-8 --

import argparse
from ns3gym import ns3env
import random
from random import randrange

_author_ = "Piotr Gawlowicz"
_copyright_ = "Copyright (c) 2018, Technische Universität Berlin"
_version_ = "0.1.0"
_email_ = "gawlowicz@tkn.tu-berlin.de"


parser = argparse.ArgumentParser(description='Start simulation script on/off')
parser.add_argument('--start',
                    type=int,
                    default=1,
                    help='Start ns-3 simulation script 0/1, Default: 1')
parser.add_argument('--iterations',
                    type=int,
                    default=1,
                    help='Number of iterations, Default: 1')
args = parser.parse_args()
startSim = bool(args.start)
iterationNum = int(args.iterations)

port = 5555
simTime = 20  # seconds
stepTime = 0.5  # seconds
seed = 0
simArgs = {"--simTime": simTime,
           "--testArg": 123}
debug = False

env = ns3env.Ns3Env(port=port, stepTime=stepTime, startSim=startSim,
                    simSeed=seed, simArgs=simArgs, debug=debug)
# simpler:
#env = ns3env.Ns3Env()
env.reset()

ob_space = env.observation_space
ac_space = env.action_space
print("Observation space: ", ob_space,  ob_space.dtype)
print("Action space: ", ac_space, ac_space.dtype)

stepIdx = 0
currIt = 0
try:
    while True:
        print("Start iteration: ", currIt)
        obs = env.reset()
        print("Step: ", stepIdx)
        print("---obs:", obs)
        action = []
        for el in obs:
            action.append(el)
        visited = []
        str1 = ''.join(str(e) for e in action)
        visited.append(str1)   

        while True:
            stepIdx += 1

            print("---action: ", action)
            number_towers = int(len(obs)/3)

            graph = [[] for _ in range(number_towers)]

            for i, node in enumerate(obs):
                graph[node].append(i)

            # random.choice(sequence)
            while True:
                chosen_tower = randrange(number_towers)
                candidates = []

                for i in range(number_towers):
                    if i == chosen_tower:
                        continue
                    for ue in graph[i]:
                        candidates.append(ue)

                chosen_ue = random.choice(candidates)
                print(chosen_ue)
                old_connection = action[chosen_ue]
                action[chosen_ue] = chosen_tower

                str1 = ''.join(str(e) for e in action)
                if str1 in visited:
                    action[chosen_ue] = old_connection
                    continue
                else:
                    visited.append(str1)
                    break

            print("new action", action)
            print("Step: ", stepIdx)

            obs, reward, done, info = env.step(action)

            print("---obs, reward, done, info: ", obs, reward, done, info)

            if done:
                stepIdx = 0
                if currIt + 1 < iterationNum:
                    env.reset()
                break
            if reward < 0:
                action[chosen_ue] = old_connection
        currIt += 1
        if currIt == iterationNum:
            break

except KeyboardInterrupt:
    print("Ctrl-C -> Exit")
finally:
    env.close()
    print("Done")
