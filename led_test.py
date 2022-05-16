#!/usr/bin/env python3
"""Sample script to run a few colour tests on the strip."""
from apa102_pi.colorschemes import colorschemes

NUM_LED = 3
BRIGHTNESS = 31


def main():
    # Five quick trips through the rainbow
    my_cycle = colorschemes.Rainbow(num_led=NUM_LED, pause_value=0.1,
                                         num_steps_per_cycle=30, num_cycles=-1, order='rgb',
                                         global_brightness=BRIGHTNESS)
    my_cycle.start()

    print('Finished the test')


if __name__ == '__main__':
    main()