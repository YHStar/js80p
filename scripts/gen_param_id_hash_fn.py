###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
#
# JS80P is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# JS80P is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###############################################################################

import random
import sys
import time


# Inspiration from https://orlp.net/blog/worlds-smallest-hash-table/


LETTER_OFFSET = ord("A") - 10   # 0x41 - 10
DIGIT_OFFSET = ord("0")         # 0x30

params = [
    "AM",
    "CAMP",
    "CC1",
    "CC10",
    "CC2",
    "CC3",
    "CC4",
    "CC5",
    "CC6",
    "CC7",
    "CC8",
    "CC9",
    "CDG",
    "CDTN",
    "CF1FIA",
    "CF1FRQ",
    "CF1G",
    "CF1LOG",
    "CF1Q",
    "CF1QIA",
    "CF1QLG",
    "CF1TYP",
    "CF2FIA",
    "CF2FRQ",
    "CF2G",
    "CF2LOG",
    "CF2Q",
    "CF2QIA",
    "CF2QLG",
    "CF2TYP",
    "CFIN",
    "CFLD",
    "COIA",
    "COIS",
    "CPAN",
    "CPRD",
    "CPRT",
    "CTUN",
    "CVOL",
    "CVS",
    "CWAV",
    "CWID",
    "ECDEL",
    "ECDF",
    "ECDG",
    "ECDPT",
    "ECDRY",
    "ECFB",
    "ECFRQ",
    "ECHPF",
    "ECHPQ",
    "ECLHQ",
    "ECLLG",
    "ECLOG",
    "ECSYN",
    "ECTYP",
    "ECWET",
    "ECWID",
    "EDG",
    "EECAT",
    "EECR",
    "EECRL",
    "EECTH",
    "EEDEL",
    "EEDF",
    "EEDG",
    "EEDRY",
    "EEDST",
    "EEFB",
    "EEHPF",
    "EEHPQ",
    "EEINV",
    "EELHQ",
    "EELOG",
    "EESYN",
    "EEWET",
    "EEWID",
    "EF1FRQ",
    "EF1G",
    "EF1LOG",
    "EF1Q",
    "EF1QLG",
    "EF1TYP",
    "EF2FRQ",
    "EF2G",
    "EF2LOG",
    "EF2Q",
    "EF2QLG",
    "EF2TYP",
    "EOG",
    "ERCAT",
    "ERCR",
    "ERCRL",
    "ERCTH",
    "ERDF",
    "ERDG",
    "ERDRY",
    "ERDST",
    "ERHPF",
    "ERHPQ",
    "ERLHQ",
    "ERLOG",
    "ERRS",
    "ERTYP",
    "ERWET",
    "ERWID",
    "EV1V",
    "EV2V",
    "EV3V",
    "F10AMT",
    "F10DSH",
    "F10DST",
    "F10IN",
    "F10MAX",
    "F10MIN",
    "F10RND",
    "F11AMT",
    "F11DSH",
    "F11DST",
    "F11IN",
    "F11MAX",
    "F11MIN",
    "F11RND",
    "F12AMT",
    "F12DSH",
    "F12DST",
    "F12IN",
    "F12MAX",
    "F12MIN",
    "F12RND",
    "F13AMT",
    "F13DSH",
    "F13DST",
    "F13IN",
    "F13MAX",
    "F13MIN",
    "F13RND",
    "F14AMT",
    "F14DSH",
    "F14DST",
    "F14IN",
    "F14MAX",
    "F14MIN",
    "F14RND",
    "F15AMT",
    "F15DSH",
    "F15DST",
    "F15IN",
    "F15MAX",
    "F15MIN",
    "F15RND",
    "F16AMT",
    "F16DSH",
    "F16DST",
    "F16IN",
    "F16MAX",
    "F16MIN",
    "F16RND",
    "F17AMT",
    "F17DSH",
    "F17DST",
    "F17IN",
    "F17MAX",
    "F17MIN",
    "F17RND",
    "F18AMT",
    "F18DSH",
    "F18DST",
    "F18IN",
    "F18MAX",
    "F18MIN",
    "F18RND",
    "F19AMT",
    "F19DSH",
    "F19DST",
    "F19IN",
    "F19MAX",
    "F19MIN",
    "F19RND",
    "F1AMT",
    "F1DSH",
    "F1DST",
    "F1IN",
    "F1MAX",
    "F1MIN",
    "F1RND",
    "F20AMT",
    "F20DSH",
    "F20DST",
    "F20IN",
    "F20MAX",
    "F20MIN",
    "F20RND",
    "F21AMT",
    "F21DSH",
    "F21DST",
    "F21IN",
    "F21MAX",
    "F21MIN",
    "F21RND",
    "F22AMT",
    "F22DSH",
    "F22DST",
    "F22IN",
    "F22MAX",
    "F22MIN",
    "F22RND",
    "F23AMT",
    "F23DSH",
    "F23DST",
    "F23IN",
    "F23MAX",
    "F23MIN",
    "F23RND",
    "F24AMT",
    "F24DSH",
    "F24DST",
    "F24IN",
    "F24MAX",
    "F24MIN",
    "F24RND",
    "F25AMT",
    "F25DSH",
    "F25DST",
    "F25IN",
    "F25MAX",
    "F25MIN",
    "F25RND",
    "F26AMT",
    "F26DSH",
    "F26DST",
    "F26IN",
    "F26MAX",
    "F26MIN",
    "F26RND",
    "F27AMT",
    "F27DSH",
    "F27DST",
    "F27IN",
    "F27MAX",
    "F27MIN",
    "F27RND",
    "F28AMT",
    "F28DSH",
    "F28DST",
    "F28IN",
    "F28MAX",
    "F28MIN",
    "F28RND",
    "F29AMT",
    "F29DSH",
    "F29DST",
    "F29IN",
    "F29MAX",
    "F29MIN",
    "F29RND",
    "F2AMT",
    "F2DSH",
    "F2DST",
    "F2IN",
    "F2MAX",
    "F2MIN",
    "F2RND",
    "F30AMT",
    "F30DSH",
    "F30DST",
    "F30IN",
    "F30MAX",
    "F30MIN",
    "F30RND",
    "F3AMT",
    "F3DSH",
    "F3DST",
    "F3IN",
    "F3MAX",
    "F3MIN",
    "F3RND",
    "F4AMT",
    "F4DSH",
    "F4DST",
    "F4IN",
    "F4MAX",
    "F4MIN",
    "F4RND",
    "F5AMT",
    "F5DSH",
    "F5DST",
    "F5IN",
    "F5MAX",
    "F5MIN",
    "F5RND",
    "F6AMT",
    "F6DSH",
    "F6DST",
    "F6IN",
    "F6MAX",
    "F6MIN",
    "F6RND",
    "F7AMT",
    "F7DSH",
    "F7DST",
    "F7IN",
    "F7MAX",
    "F7MIN",
    "F7RND",
    "F8AMT",
    "F8DSH",
    "F8DST",
    "F8IN",
    "F8MAX",
    "F8MIN",
    "F8RND",
    "F9AMT",
    "F9DSH",
    "F9DST",
    "F9IN",
    "F9MAX",
    "F9MIN",
    "F9RND",
    "FM",
    "L1AEN",
    "L1AMT",
    "L1CEN",
    "L1DST",
    "L1FRQ",
    "L1LOG",
    "L1MAX",
    "L1MIN",
    "L1PHS",
    "L1RND",
    "L1SYN",
    "L1WAV",
    "L2AEN",
    "L2AMT",
    "L2CEN",
    "L2DST",
    "L2FRQ",
    "L2LOG",
    "L2MAX",
    "L2MIN",
    "L2PHS",
    "L2RND",
    "L2SYN",
    "L2WAV",
    "L3AEN",
    "L3AMT",
    "L3CEN",
    "L3DST",
    "L3FRQ",
    "L3LOG",
    "L3MAX",
    "L3MIN",
    "L3PHS",
    "L3RND",
    "L3SYN",
    "L3WAV",
    "L4AEN",
    "L4AMT",
    "L4CEN",
    "L4DST",
    "L4FRQ",
    "L4LOG",
    "L4MAX",
    "L4MIN",
    "L4PHS",
    "L4RND",
    "L4SYN",
    "L4WAV",
    "L5AEN",
    "L5AMT",
    "L5CEN",
    "L5DST",
    "L5FRQ",
    "L5LOG",
    "L5MAX",
    "L5MIN",
    "L5PHS",
    "L5RND",
    "L5SYN",
    "L5WAV",
    "L6AEN",
    "L6AMT",
    "L6CEN",
    "L6DST",
    "L6FRQ",
    "L6LOG",
    "L6MAX",
    "L6MIN",
    "L6PHS",
    "L6RND",
    "L6SYN",
    "L6WAV",
    "L7AEN",
    "L7AMT",
    "L7CEN",
    "L7DST",
    "L7FRQ",
    "L7LOG",
    "L7MAX",
    "L7MIN",
    "L7PHS",
    "L7RND",
    "L7SYN",
    "L7WAV",
    "L8AEN",
    "L8AMT",
    "L8CEN",
    "L8DST",
    "L8FRQ",
    "L8LOG",
    "L8MAX",
    "L8MIN",
    "L8PHS",
    "L8RND",
    "L8SYN",
    "L8WAV",
    "MAMP",
    "MC1",
    "MC10",
    "MC2",
    "MC3",
    "MC4",
    "MC5",
    "MC6",
    "MC7",
    "MC8",
    "MC9",
    "MDTN",
    "MF1FIA",
    "MF1FRQ",
    "MF1G",
    "MF1LOG",
    "MF1Q",
    "MF1QIA",
    "MF1QLG",
    "MF1TYP",
    "MF2FIA",
    "MF2FRQ",
    "MF2G",
    "MF2LOG",
    "MF2Q",
    "MF2QIA",
    "MF2QLG",
    "MF2TYP",
    "MFIN",
    "MFLD",
    "MIX",
    "MODE",
    "MOIA",
    "MOIS",
    "MPAN",
    "MPRD",
    "MPRT",
    "MSUB",
    "MTUN",
    "MVOL",
    "MVS",
    "MWAV",
    "MWID",
    "N10AMT",
    "N10ASH",
    "N10ATK",
    "N10DEC",
    "N10DEL",
    "N10DSH",
    "N10FIN",
    "N10HLD",
    "N10INI",
    "N10PK",
    "N10REL",
    "N10RSH",
    "N10SUS",
    "N10SYN",
    "N10TIN",
    "N10UPD",
    "N10VIN",
    "N11AMT",
    "N11ASH",
    "N11ATK",
    "N11DEC",
    "N11DEL",
    "N11DSH",
    "N11FIN",
    "N11HLD",
    "N11INI",
    "N11PK",
    "N11REL",
    "N11RSH",
    "N11SUS",
    "N11SYN",
    "N11TIN",
    "N11UPD",
    "N11VIN",
    "N12AMT",
    "N12ASH",
    "N12ATK",
    "N12DEC",
    "N12DEL",
    "N12DSH",
    "N12FIN",
    "N12HLD",
    "N12INI",
    "N12PK",
    "N12REL",
    "N12RSH",
    "N12SUS",
    "N12SYN",
    "N12TIN",
    "N12UPD",
    "N12VIN",
    "N1AMT",
    "N1ASH",
    "N1ATK",
    "N1DEC",
    "N1DEL",
    "N1DSH",
    "N1FIN",
    "N1HLD",
    "N1INI",
    "N1PK",
    "N1REL",
    "N1RSH",
    "N1SUS",
    "N1SYN",
    "N1TIN",
    "N1UPD",
    "N1VIN",
    "N2AMT",
    "N2ASH",
    "N2ATK",
    "N2DEC",
    "N2DEL",
    "N2DSH",
    "N2FIN",
    "N2HLD",
    "N2INI",
    "N2PK",
    "N2REL",
    "N2RSH",
    "N2SUS",
    "N2SYN",
    "N2TIN",
    "N2UPD",
    "N2VIN",
    "N3AMT",
    "N3ASH",
    "N3ATK",
    "N3DEC",
    "N3DEL",
    "N3DSH",
    "N3FIN",
    "N3HLD",
    "N3INI",
    "N3PK",
    "N3REL",
    "N3RSH",
    "N3SUS",
    "N3SYN",
    "N3TIN",
    "N3UPD",
    "N3VIN",
    "N4AMT",
    "N4ASH",
    "N4ATK",
    "N4DEC",
    "N4DEL",
    "N4DSH",
    "N4FIN",
    "N4HLD",
    "N4INI",
    "N4PK",
    "N4REL",
    "N4RSH",
    "N4SUS",
    "N4SYN",
    "N4TIN",
    "N4UPD",
    "N4VIN",
    "N5AMT",
    "N5ASH",
    "N5ATK",
    "N5DEC",
    "N5DEL",
    "N5DSH",
    "N5FIN",
    "N5HLD",
    "N5INI",
    "N5PK",
    "N5REL",
    "N5RSH",
    "N5SUS",
    "N5SYN",
    "N5TIN",
    "N5UPD",
    "N5VIN",
    "N6AMT",
    "N6ASH",
    "N6ATK",
    "N6DEC",
    "N6DEL",
    "N6DSH",
    "N6FIN",
    "N6HLD",
    "N6INI",
    "N6PK",
    "N6REL",
    "N6RSH",
    "N6SUS",
    "N6SYN",
    "N6TIN",
    "N6UPD",
    "N6VIN",
    "N7AMT",
    "N7ASH",
    "N7ATK",
    "N7DEC",
    "N7DEL",
    "N7DSH",
    "N7FIN",
    "N7HLD",
    "N7INI",
    "N7PK",
    "N7REL",
    "N7RSH",
    "N7SUS",
    "N7SYN",
    "N7TIN",
    "N7UPD",
    "N7VIN",
    "N8AMT",
    "N8ASH",
    "N8ATK",
    "N8DEC",
    "N8DEL",
    "N8DSH",
    "N8FIN",
    "N8HLD",
    "N8INI",
    "N8PK",
    "N8REL",
    "N8RSH",
    "N8SUS",
    "N8SYN",
    "N8TIN",
    "N8UPD",
    "N8VIN",
    "N9AMT",
    "N9ASH",
    "N9ATK",
    "N9DEC",
    "N9DEL",
    "N9DSH",
    "N9FIN",
    "N9HLD",
    "N9INI",
    "N9PK",
    "N9REL",
    "N9RSH",
    "N9SUS",
    "N9SYN",
    "N9TIN",
    "N9UPD",
    "N9VIN",
    "PM",
    "POLY",
]


def main(argv):
    # for name in params:
        # h = compute_hash(name, 23781, 9, 128)
        # print(
            # f"assert_eq(Synth::ParamId::{name}, synth.find_param_id(\"{name}\"));"
        # )

    # return 0

    best_utilized = 0
    best_mod = 2 ** 31
    best_max_coll = 2 ** 31
    best_avg_len = 2 ** 31
    start = time.time()
    delta = 0

    for i in range(1, 100000):
        # multiplier = random.randrange(2 ** 15)
        multiplier = i
        multiplier = multiplier * 2 + 1

        for shift in range(23):
            # for mod in range(220, 290):
            for mod in [128, 256]:
                hashes = {}

                for name in params:
                    h = compute_hash(name, multiplier, shift, mod)
                    hashes.setdefault(h, []).append(name)

                max_coll = max(len(n) for n in hashes.values())
                avg_len = [len(n) for n in hashes.values()]
                avg_len = sum(avg_len) / len(avg_len)
                utilized = len(hashes.keys())

                if (
                    (max_coll < best_max_coll)
                    or (max_coll == best_max_coll and avg_len < best_avg_len)
                ):
                    delta_t = time.time() - start
                    best_utilized = utilized
                    best_max_coll = max_coll
                    best_avg_len = avg_len
                    best_mod = mod
                    print(
                        "\t".join([
                            f"time={delta_t}",
                            f"multiplier={multiplier}",
                            f"shift={shift}",
                            f"max_coll={max_coll}",
                            f"avg_len={avg_len}",
                            f"mod={mod}",
                            f"utilized={len(hashes)}"
                        ])
                    )


def compute_hash(text: str, multiplier: int, shift: int, mod: int) -> int:
    h = 0

    for i, c in enumerate(text):
        c = ord(c)

        if c >= LETTER_OFFSET:
            c -= LETTER_OFFSET
        else:
            c -= DIGIT_OFFSET

        h *= 36
        h += c

        if i == 4:
            break

    h <<= 3
    h += i
    h = (h * multiplier) >> shift
    h = h % mod

    return h


if __name__ == "__main__":
    sys.exit(main(sys.argv))
