m = int("fffffffe7ffbfa37", 16)
import signal
for s in range(1, 65):
    if m & (1 << (s-1)):
        try:
            print(s, signal.Signals(s).name)
        except ValueError:
            print(s)