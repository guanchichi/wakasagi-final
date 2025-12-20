#!/usr/bin/env python
import subprocess
import multiprocessing
import argparse
import sys
import re
import os
import fcntl
import time
import resource
import traceback
from pathlib import Path

def play_one_game(args):
    referee_path, agent1_path, agent2_path, show_detail, game_id, time_per_game = args

    referee_path = Path(referee_path)
    agent1_path = Path(agent1_path)
    agent2_path = Path(agent2_path)

    if not agent1_path.is_absolute():
        agent1_path = Path.cwd() / agent1_path
    if not agent2_path.is_absolute():
        agent2_path = Path.cwd() / agent2_path
    if not referee_path.is_absolute():
        referee_path = Path.cwd() / referee_path

    def make_nonblocking(f):
        fd = f.fileno()
        flags = fcntl.fcntl(fd, fcntl.F_GETFL)
        fcntl.fcntl(fd, fcntl.F_SETFL, flags | os.O_NONBLOCK)

    def readline(proc, timeout=5):
        make_nonblocking(proc.stdout)

        buf = []
        end = time.time() + timeout

        while time.time() < end:
            try:
                chunk = proc.stdout.read(1)
            except:
                chunk = None

            if chunk:
                if chunk == "\n":
                    return "".join(buf), end - time.time()
                buf.append(chunk)
            else:
                time.sleep(0.01)

        return None, 0  # timeout

    def set_limit():
        LIMIT = 1024 * 1024 * 1024
        resource.setrlimit(resource.RLIMIT_AS, (LIMIT, LIMIT))

    try:
        # Start referee and agents
        ref = subprocess.Popen(
            [str(referee_path)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            text=True
        )
        a1 = subprocess.Popen(
            [str(agent1_path)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, preexec_fn=set_limit, bufsize=0
        )
        a2 = subprocess.Popen(
            [str(agent2_path)],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, preexec_fn=set_limit, bufsize=0
        )

        # timers
        red_timer = time_per_game
        black_timer = time_per_game

        ply = 1

        if game_id % 2 == 1:
            red = a1
            black = a2
            red_str = "Agent 1"
            black_str = "Agent 2"
        else:
            red = a2
            black = a1
            red_str = "Agent 2"
            black_str = "Agent 1"

        ref.stdin.write("START 1\n")
        ref.stdin.flush()

        last_color = 'b' # init

        while True:
            msg, _ = readline(ref, timeout=1)
            #print(f"<r '{msg}'")
            if msg is None or msg.startswith("ERR"):
                break

            # get state
            #print(f">r 'STATE'")
            ref.stdin.write("STATE\n")
            ref.stdin.flush()
            status, _ = readline(ref, timeout=1)
            #print(f"<r '{status}'")

            if status == "IN-PLAY":
                board, _ = readline(ref, timeout=1)
                ok, _ = readline(ref, timeout=1)
                current_color = board.split()[1]

            elif status == "RED WINS":
                board, _ = readline(ref, timeout=1)
                reason, _ = readline(ref, timeout=1)
                if show_detail:
                    print(f"Game #{game_id}: \033[1;31m{red_str}\033[0m won by {reason}")
                return red_str, 'r'

            elif status == "BLACK WINS":
                board, _ = readline(ref, timeout=1)
                reason, _ = readline(ref, timeout=1)
                if show_detail:
                    print(f"Game #{game_id}: \033[1;30m{black_str}\033[0m won by {reason}")
                return black_str, 'b'

            elif status == "DRAW":
                board, _ = readline(ref, timeout=1)
                reason, _ = readline(ref, timeout=1)
                if show_detail:
                    print(f"Game #{game_id}: \033[1;32mdrawn\033[0m by {reason}")
                return "draw", 'd'

            else:
                print("???")
                break

            if current_color == last_color:
                # reassign side
                if game_id % 2 == 0:
                    red = a1
                    black = a2
                    red_str = "Agent 1"
                    black_str = "Agent 2"
                else:
                    red = a2
                    black = a1
                    red_str = "Agent 2"
                    black_str = "Agent 1"

                temp = black_timer
                black_timer = red_timer
                red_timer = temp

            last_color = current_color

            # the engine says so
            to_play = red if current_color == 'r' else black
            not_to_play = black if current_color == 'r' else red

            # check agent status
            if to_play.poll() is not None:
                if show_detail:
                    print(f"Game #{game_id}: {red_str if current_color == 'r' else black_str} terminated unexpectedly.")
                return (black_str, 'b') if current_color == 'r' else (red_str, 'r')

            # send board to other agent
            not_to_play.stdin.write(f"{board} -9999 -9999\n")
            not_to_play.stdin.flush()

            # get moves from agent
            #print(f">a '{board} {red_timer * 1000} {black_timer * 1000}'")
            to_play.stdin.write(f"{board} {red_timer * 1000} {black_timer * 1000}\n")
            to_play.stdin.flush()
            if current_color == 'r':
                do_move, red_timer = readline(to_play, timeout=red_timer)
            else:
                do_move, black_timer = readline(to_play, timeout=black_timer)
            #print(f"<a '{do_move}'")

            if do_move is None:
                if show_detail:
                    print(f"Game #{game_id}: {red_str if current_color == 'r' else black_str} did not respond in time.")
                return (black_str, 'b') if current_color == 'r' else (red_str, 'r')

            # submit move
            #print(f">r '{do_move}'")
            ref.stdin.write(do_move + '\n')
            ref.stdin.flush()

            # increment
            ply += 1

        ref.kill(); a1.kill(); a2.kill()
        return "wtf unreachable", ''

    except Exception as e:
        print(traceback.format_exc())
        return f"except - {e}", ''

def main():
    parser = argparse.ArgumentParser(description="Run multiple games between two agents using a referee.")
    parser.add_argument("referee", type=Path, help="Path to referee executable")
    parser.add_argument("agent1", type=Path, help="Path to first agent")
    parser.add_argument("agent2", type=Path, help="Path to second agent")
    parser.add_argument("-n", "--num-games", type=int, default=10)
    parser.add_argument("-j", "--jobs", type=int, default=multiprocessing.cpu_count())
    parser.add_argument("-s", "--show-detail", type=int, default=1, help="Show results of each match.")
    parser.add_argument("-t", "--time", type=int, default=600, help="Time in second per game per side")
    args = parser.parse_args()

    jobs = [(str(args.referee), str(args.agent1), str(args.agent2), args.show_detail, i, args.time) for i in range(args.num_games)]

    with multiprocessing.Pool(args.jobs) as pool:
        results = pool.map(play_one_game, jobs)

    counts = {}
    rsplit = {'Agent 1': 0, 'Agent 2': 0}
    for r, s in results:
        counts[r] = counts.get(r, 0) + 1
        if r == 'Agent 1' or r == 'Agent 2':
            if s == 'r':
                rsplit[r] += 1

    print(f"=== Results over {args.num_games} games ===")
    for k, v in counts.items():
        print(f"  {k}: {v} {f"({rsplit[k]} Red)" if k in rsplit else ''}")

    print("==============================")
    a2_score = counts.get('Agent 2', 0) + 0.5 * counts.get('draw', 0)
    a1_score = counts.get('Agent 1', 0) + 0.5 * counts.get('draw', 0)
    print(f"{'Agent 1':<10}{a1_score} - {a2_score}{'Agent 2':>10}")

if __name__ == "__main__":
    main()