/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JS80P__SYNTH_HPP
#define JS80P__SYNTH_HPP

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

#include "js80p.hpp"
#include "midi.hpp"
#include "note_stack.hpp"
#include "spscqueue.hpp"
#include "voice.hpp"

#include "dsp/envelope.hpp"
#include "dsp/biquad_filter.hpp"
#include "dsp/chorus.hpp"
#include "dsp/delay.hpp"
#include "dsp/distortion.hpp"
#include "dsp/echo.hpp"
#include "dsp/effect.hpp"
#include "dsp/effects.hpp"
#include "dsp/filter.hpp"
#include "dsp/gain.hpp"
#include "dsp/lfo.hpp"
#include "dsp/lfo_envelope_list.hpp"
#include "dsp/macro.hpp"
#include "dsp/math.hpp"
#include "dsp/midi_controller.hpp"
#include "dsp/mixer.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/param.hpp"
#include "dsp/peak_tracker.hpp"
#include "dsp/reverb.hpp"
#include "dsp/queue.hpp"
#include "dsp/side_chain_compressable_effect.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

/**
 * \warning Calling any method of a \c Synth object or its members outside the
 *          audio thread is not safe, unless indicated otherwise.
 */
class Synth : public Midi::EventHandler, public SignalProducer
{
    friend class SignalProducer;

    private:
        static constexpr Integer VOICE_INDEX_MASK = 0x3f;

    public:
        static constexpr Integer POLYPHONY = VOICE_INDEX_MASK + 1;

        static constexpr Integer OUT_CHANNELS = Carrier::CHANNELS;

        static constexpr Integer ENVELOPE_FLOAT_PARAMS = 12;
        static constexpr Integer ENVELOPE_DISCRETE_PARAMS = 5;

        static constexpr Integer MIDI_CONTROLLERS = 128;

        static constexpr Integer MACROS = 30;
        static constexpr Integer MACRO_PARAMS = 7;

        enum MessageType {
            SET_PARAM = 1,          ///< Set the given parameter's ratio to
                                    ///< \c number_param.

            ASSIGN_CONTROLLER = 2,  ///< Assign the controller identified by
                                    ///< \c byte_param to the given parameter.

            REFRESH_PARAM = 3,      ///< Make sure that \c get_param_ratio_atomic()
                                    ///< will return the most recent value of
                                    ///< the given parameter.

            CLEAR = 4,              ///< Clear all buffers, release all
                                    ///< controller assignments, and reset all
                                    ///< parameters to their default values.

            CLEAR_DIRTY_FLAG = 5,   ///< Clear the dirty flag.

            INVALID_MESSAGE_TYPE,
        };

        enum ParamId {
            MIX = 0,         ///< Modulator Additive Volume
            PM = 1,          ///< Phase Modulation
            FM = 2,          ///< Frequency Modulation
            AM = 3,          ///< Amplitude Modulation

            MAMP = 4,        ///< Modulator Amplitude
            MVS = 5,         ///< Modulator Velocity Sensitivity
            MFLD = 6,        ///< Modulator Folding
            MPRT = 7,        ///< Modulator Portamento Length
            MPRD = 8,        ///< Modulator Portamento Depth
            MDTN = 9,        ///< Modulator Detune
            MFIN = 10,       ///< Modulator Fine Detune
            MWID = 11,       ///< Modulator Width
            MPAN = 12,       ///< Modulator Pan
            MVOL = 13,       ///< Modulator Volume
            MSUB = 14,       ///< Modulator Subharmonic Amplitude
            MC1 = 15,        ///< Modulator Custom Waveform 1st Harmonic
            MC2 = 16,        ///< Modulator Custom Waveform 2nd Harmonic
            MC3 = 17,        ///< Modulator Custom Waveform 3rd Harmonic
            MC4 = 18,        ///< Modulator Custom Waveform 4th Harmonic
            MC5 = 19,        ///< Modulator Custom Waveform 5th Harmonic
            MC6 = 20,        ///< Modulator Custom Waveform 6th Harmonic
            MC7 = 21,        ///< Modulator Custom Waveform 7th Harmonic
            MC8 = 22,        ///< Modulator Custom Waveform 8th Harmonic
            MC9 = 23,        ///< Modulator Custom Waveform 9th Harmonic
            MC10 = 24,       ///< Modulator Custom Waveform 10th Harmonic
            MF1FRQ = 25,     ///< Modulator Filter 1 Frequency
            MF1Q = 26,       ///< Modulator Filter 1 Q Factor
            MF1G = 27,       ///< Modulator Filter 1 Gain
            MF1FIA = 28,     ///< Modulator Filter 1 Frequency Inaccuracy
            MF1QIA = 29,     ///< Modulator Filter 1 Q Factor Inaccuracy
            MF2FRQ = 30,     ///< Modulator Filter 2 Frequency
            MF2Q = 31,       ///< Modulator Filter 2 Q Factor
            MF2G = 32,       ///< Modulator Filter 2 Gain
            MF2FIA = 33,     ///< Modulator Filter 2 Frequency Inaccuracy
            MF2QIA = 34,     ///< Modulator Filter 2 Q Factor Inaccuracy

            CAMP = 35,       ///< Carrier Amplitude
            CVS = 36,        ///< Carrier Velocity Sensitivity
            CFLD = 37,       ///< Carrier Folding
            CPRT = 38,       ///< Carrier Portamento Length
            CPRD = 39,       ///< Carrier Portamento Depth
            CDTN = 40,       ///< Carrier Detune
            CFIN = 41,       ///< Carrier Fine Detune
            CWID = 42,       ///< Carrier Width
            CPAN = 43,       ///< Carrier Pan
            CVOL = 44,       ///< Carrier Volume
            CDG = 45,        ///< Carrier Distortion Gain
            CC1 = 46,        ///< Carrier Custom Waveform 1st Harmonic
            CC2 = 47,        ///< Carrier Custom Waveform 2nd Harmonic
            CC3 = 48,        ///< Carrier Custom Waveform 3rd Harmonic
            CC4 = 49,        ///< Carrier Custom Waveform 4th Harmonic
            CC5 = 50,        ///< Carrier Custom Waveform 5th Harmonic
            CC6 = 51,        ///< Carrier Custom Waveform 6th Harmonic
            CC7 = 52,        ///< Carrier Custom Waveform 7th Harmonic
            CC8 = 53,        ///< Carrier Custom Waveform 8th Harmonic
            CC9 = 54,        ///< Carrier Custom Waveform 9th Harmonic
            CC10 = 55,       ///< Carrier Custom Waveform 10th Harmonic
            CF1FRQ = 56,     ///< Carrier Filter 1 Frequency
            CF1Q = 57,       ///< Carrier Filter 1 Q Factor
            CF1G = 58,       ///< Carrier Filter 1 Gain
            CF1FIA = 59,     ///< Carrier Filter 1 Frequency Inaccuracy
            CF1QIA = 60,     ///< Carrier Filter 1 Q Factor Inaccuracy
            CF2FRQ = 61,     ///< Carrier Filter 2 Frequency
            CF2Q = 62,       ///< Carrier Filter 2 Q Factor
            CF2G = 63,       ///< Carrier Filter 2 Gain
            CF2FIA = 64,     ///< Carrier Filter 2 Frequency Inaccuracy
            CF2QIA = 65,     ///< Carrier Filter 2 Q Factor Inaccuracy

            EV1V = 66,       ///< Effects Volume 1
            EOG = 67,        ///< Effects Overdrive Gain
            EDG = 68,        ///< Effects Distortion Gain
            EF1FRQ = 69,     ///< Effects Filter 1 Frequency
            EF1Q = 70,       ///< Effects Filter 1 Q Factor
            EF1G = 71,       ///< Effects Filter 1 Gain
            EF2FRQ = 72,     ///< Effects Filter 2 Frequency
            EF2Q = 73,       ///< Effects Filter 2 Q Factor
            EF2G = 74,       ///< Effects Filter 2 Gain
            EV2V = 75,       ///< Effects Volume 2
            ECDEL = 76,      ///< Effects Chorus Delay
            ECFRQ = 77,      ///< Effects Chorus LFO Frequency
            ECDPT = 78,      ///< Effects Chorus Depth
            ECFB = 79,       ///< Effects Chorus Feedback
            ECDF = 80,       ///< Effects Chorus Dampening Frequency
            ECDG = 81,       ///< Effects Chorus Dampening Gain
            ECWID = 82,      ///< Effects Chorus Stereo Width
            ECHPF = 83,      ///< Effects Chorus Highpass Frequency
            ECHPQ = 84,      ///< Effects Chorus Highpass Q Factor
            ECWET = 85,      ///< Effects Chorus Wet Volume
            ECDRY = 86,      ///< Effects Chorus Dry Volume
            EEDEL = 87,      ///< Effects Echo Delay
            EEINV = 88,      ///< Effects Echo Input Volume
            EEFB = 89,       ///< Effects Echo Feedback
            EEDST = 90,      ///< Effects Echo Distortion
            EEDF = 91,       ///< Effects Echo Dampening Frequency
            EEDG = 92,       ///< Effects Echo Dampening Gain
            EEWID = 93,      ///< Effects Echo Stereo Width
            EEHPF = 94,      ///< Effects Echo Highpass Frequency
            EEHPQ = 95,      ///< Effects Echo Highpass Q Factor
            EECTH = 96,      ///< Effects Echo Side-Chain Compression Threshold
            EECAT = 97,      ///< Effects Echo Side-Chain Compression Attack Time
            EECRL = 98,      ///< Effects Echo Side-Chain Compression Release Time
            EECR = 99,       ///< Effects Echo Side-Chain Compression Ratio
            EEWET = 100,     ///< Effects Echo Wet Volume
            EEDRY = 101,     ///< Effects Echo Dry Volume
            ERRS = 102,      ///< Effects Reverb Room Size
            ERDST = 103,     ///< Effects Reverb Distortion
            ERDF = 104,      ///< Effects Reverb Dampening Frequency
            ERDG = 105,      ///< Effects Reverb Dampening Gain
            ERWID = 106,     ///< Effects Reverb Stereo Width
            ERHPF = 107,     ///< Effects Reverb Highpass Frequency
            ERHPQ = 108,     ///< Effects Reverb Highpass Q Factor
            ERCTH = 109,     ///< Effects Reverb Side-Chain Compression Threshold
            ERCAT = 110,     ///< Effects Reverb Side-Chain Compression Attack Time
            ERCRL = 111,     ///< Effects Reverb Side-Chain Compression Release Time
            ERCR = 112,      ///< Effects Reverb Side-Chain Compression Ratio
            ERWET = 113,     ///< Effects Reverb Wet Volume
            ERDRY = 114,     ///< Effects Reverb Dry Volume
            EV3V = 115,      ///< Effects Volume 3

            M1IN = 116,      ///< Macro 1 Input
            M1MIN = 117,     ///< Macro 1 Minimum Value
            M1MAX = 118,     ///< Macro 1 Maximum Value
            M1AMT = 119,     ///< Macro 1 Amount
            M1DST = 120,     ///< Macro 1 Distortion
            M1RND = 121,     ///< Macro 1 Randomness

            M2IN = 122,      ///< Macro 2 Input
            M2MIN = 123,     ///< Macro 2 Minimum Value
            M2MAX = 124,     ///< Macro 2 Maximum Value
            M2AMT = 125,     ///< Macro 2 Amount
            M2DST = 126,     ///< Macro 2 Distortion
            M2RND = 127,     ///< Macro 2 Randomness

            M3IN = 128,      ///< Macro 3 Input
            M3MIN = 129,     ///< Macro 3 Minimum Value
            M3MAX = 130,     ///< Macro 3 Maximum Value
            M3AMT = 131,     ///< Macro 3 Amount
            M3DST = 132,     ///< Macro 3 Distortion
            M3RND = 133,     ///< Macro 3 Randomness

            M4IN = 134,      ///< Macro 4 Input
            M4MIN = 135,     ///< Macro 4 Minimum Value
            M4MAX = 136,     ///< Macro 4 Maximum Value
            M4AMT = 137,     ///< Macro 4 Amount
            M4DST = 138,     ///< Macro 4 Distortion
            M4RND = 139,     ///< Macro 4 Randomness

            M5IN = 140,      ///< Macro 5 Input
            M5MIN = 141,     ///< Macro 5 Minimum Value
            M5MAX = 142,     ///< Macro 5 Maximum Value
            M5AMT = 143,     ///< Macro 5 Amount
            M5DST = 144,     ///< Macro 5 Distortion
            M5RND = 145,     ///< Macro 5 Randomness

            M6IN = 146,      ///< Macro 6 Input
            M6MIN = 147,     ///< Macro 6 Minimum Value
            M6MAX = 148,     ///< Macro 6 Maximum Value
            M6AMT = 149,     ///< Macro 6 Amount
            M6DST = 150,     ///< Macro 6 Distortion
            M6RND = 151,     ///< Macro 6 Randomness

            M7IN = 152,      ///< Macro 7 Input
            M7MIN = 153,     ///< Macro 7 Minimum Value
            M7MAX = 154,     ///< Macro 7 Maximum Value
            M7AMT = 155,     ///< Macro 7 Amount
            M7DST = 156,     ///< Macro 7 Distortion
            M7RND = 157,     ///< Macro 7 Randomness

            M8IN = 158,      ///< Macro 8 Input
            M8MIN = 159,     ///< Macro 8 Minimum Value
            M8MAX = 160,     ///< Macro 8 Maximum Value
            M8AMT = 161,     ///< Macro 8 Amount
            M8DST = 162,     ///< Macro 8 Distortion
            M8RND = 163,     ///< Macro 8 Randomness

            M9IN = 164,      ///< Macro 9 Input
            M9MIN = 165,     ///< Macro 9 Minimum Value
            M9MAX = 166,     ///< Macro 9 Maximum Value
            M9AMT = 167,     ///< Macro 9 Amount
            M9DST = 168,     ///< Macro 9 Distortion
            M9RND = 169,     ///< Macro 9 Randomness

            M10IN = 170,     ///< Macro 10 Input
            M10MIN = 171,    ///< Macro 10 Minimum Value
            M10MAX = 172,    ///< Macro 10 Maximum Value
            M10AMT = 173,    ///< Macro 10 Amount
            M10DST = 174,    ///< Macro 10 Distortion
            M10RND = 175,    ///< Macro 10 Randomness

            M11IN = 176,     ///< Macro 11 Input
            M11MIN = 177,    ///< Macro 11 Minimum Value
            M11MAX = 178,    ///< Macro 11 Maximum Value
            M11AMT = 179,    ///< Macro 11 Amount
            M11DST = 180,    ///< Macro 11 Distortion
            M11RND = 181,    ///< Macro 11 Randomness

            M12IN = 182,     ///< Macro 12 Input
            M12MIN = 183,    ///< Macro 12 Minimum Value
            M12MAX = 184,    ///< Macro 12 Maximum Value
            M12AMT = 185,    ///< Macro 12 Amount
            M12DST = 186,    ///< Macro 12 Distortion
            M12RND = 187,    ///< Macro 12 Randomness

            M13IN = 188,     ///< Macro 13 Input
            M13MIN = 189,    ///< Macro 13 Minimum Value
            M13MAX = 190,    ///< Macro 13 Maximum Value
            M13AMT = 191,    ///< Macro 13 Amount
            M13DST = 192,    ///< Macro 13 Distortion
            M13RND = 193,    ///< Macro 13 Randomness

            M14IN = 194,     ///< Macro 14 Input
            M14MIN = 195,    ///< Macro 14 Minimum Value
            M14MAX = 196,    ///< Macro 14 Maximum Value
            M14AMT = 197,    ///< Macro 14 Amount
            M14DST = 198,    ///< Macro 14 Distortion
            M14RND = 199,    ///< Macro 14 Randomness

            M15IN = 200,     ///< Macro 15 Input
            M15MIN = 201,    ///< Macro 15 Minimum Value
            M15MAX = 202,    ///< Macro 15 Maximum Value
            M15AMT = 203,    ///< Macro 15 Amount
            M15DST = 204,    ///< Macro 15 Distortion
            M15RND = 205,    ///< Macro 15 Randomness

            M16IN = 206,     ///< Macro 16 Input
            M16MIN = 207,    ///< Macro 16 Minimum Value
            M16MAX = 208,    ///< Macro 16 Maximum Value
            M16AMT = 209,    ///< Macro 16 Amount
            M16DST = 210,    ///< Macro 16 Distortion
            M16RND = 211,    ///< Macro 16 Randomness

            M17IN = 212,     ///< Macro 17 Input
            M17MIN = 213,    ///< Macro 17 Minimum Value
            M17MAX = 214,    ///< Macro 17 Maximum Value
            M17AMT = 215,    ///< Macro 17 Amount
            M17DST = 216,    ///< Macro 17 Distortion
            M17RND = 217,    ///< Macro 17 Randomness

            M18IN = 218,     ///< Macro 18 Input
            M18MIN = 219,    ///< Macro 18 Minimum Value
            M18MAX = 220,    ///< Macro 18 Maximum Value
            M18AMT = 221,    ///< Macro 18 Amount
            M18DST = 222,    ///< Macro 18 Distortion
            M18RND = 223,    ///< Macro 18 Randomness

            M19IN = 224,     ///< Macro 19 Input
            M19MIN = 225,    ///< Macro 19 Minimum Value
            M19MAX = 226,    ///< Macro 19 Maximum Value
            M19AMT = 227,    ///< Macro 19 Amount
            M19DST = 228,    ///< Macro 19 Distortion
            M19RND = 229,    ///< Macro 19 Randomness

            M20IN = 230,     ///< Macro 20 Input
            M20MIN = 231,    ///< Macro 20 Minimum Value
            M20MAX = 232,    ///< Macro 20 Maximum Value
            M20AMT = 233,    ///< Macro 20 Amount
            M20DST = 234,    ///< Macro 20 Distortion
            M20RND = 235,    ///< Macro 20 Randomness

            M21IN = 236,     ///< Macro 21 Input
            M21MIN = 237,    ///< Macro 21 Minimum Value
            M21MAX = 238,    ///< Macro 21 Maximum Value
            M21AMT = 239,    ///< Macro 21 Amount
            M21DST = 240,    ///< Macro 21 Distortion
            M21RND = 241,    ///< Macro 21 Randomness

            M22IN = 242,     ///< Macro 22 Input
            M22MIN = 243,    ///< Macro 22 Minimum Value
            M22MAX = 244,    ///< Macro 22 Maximum Value
            M22AMT = 245,    ///< Macro 22 Amount
            M22DST = 246,    ///< Macro 22 Distortion
            M22RND = 247,    ///< Macro 22 Randomness

            M23IN = 248,     ///< Macro 23 Input
            M23MIN = 249,    ///< Macro 23 Minimum Value
            M23MAX = 250,    ///< Macro 23 Maximum Value
            M23AMT = 251,    ///< Macro 23 Amount
            M23DST = 252,    ///< Macro 23 Distortion
            M23RND = 253,    ///< Macro 23 Randomness

            M24IN = 254,     ///< Macro 24 Input
            M24MIN = 255,    ///< Macro 24 Minimum Value
            M24MAX = 256,    ///< Macro 24 Maximum Value
            M24AMT = 257,    ///< Macro 24 Amount
            M24DST = 258,    ///< Macro 24 Distortion
            M24RND = 259,    ///< Macro 24 Randomness

            M25IN = 260,     ///< Macro 25 Input
            M25MIN = 261,    ///< Macro 25 Minimum Value
            M25MAX = 262,    ///< Macro 25 Maximum Value
            M25AMT = 263,    ///< Macro 25 Amount
            M25DST = 264,    ///< Macro 25 Distortion
            M25RND = 265,    ///< Macro 25 Randomness

            M26IN = 266,     ///< Macro 26 Input
            M26MIN = 267,    ///< Macro 26 Minimum Value
            M26MAX = 268,    ///< Macro 26 Maximum Value
            M26AMT = 269,    ///< Macro 26 Amount
            M26DST = 270,    ///< Macro 26 Distortion
            M26RND = 271,    ///< Macro 26 Randomness

            M27IN = 272,     ///< Macro 27 Input
            M27MIN = 273,    ///< Macro 27 Minimum Value
            M27MAX = 274,    ///< Macro 27 Maximum Value
            M27AMT = 275,    ///< Macro 27 Amount
            M27DST = 276,    ///< Macro 27 Distortion
            M27RND = 277,    ///< Macro 27 Randomness

            M28IN = 278,     ///< Macro 28 Input
            M28MIN = 279,    ///< Macro 28 Minimum Value
            M28MAX = 280,    ///< Macro 28 Maximum Value
            M28AMT = 281,    ///< Macro 28 Amount
            M28DST = 282,    ///< Macro 28 Distortion
            M28RND = 283,    ///< Macro 28 Randomness

            M29IN = 284,     ///< Macro 29 Input
            M29MIN = 285,    ///< Macro 29 Minimum Value
            M29MAX = 286,    ///< Macro 29 Maximum Value
            M29AMT = 287,    ///< Macro 29 Amount
            M29DST = 288,    ///< Macro 29 Distortion
            M29RND = 289,    ///< Macro 29 Randomness

            M30IN = 290,     ///< Macro 30 Input
            M30MIN = 291,    ///< Macro 30 Minimum Value
            M30MAX = 292,    ///< Macro 30 Maximum Value
            M30AMT = 293,    ///< Macro 30 Amount
            M30DST = 294,    ///< Macro 30 Distortion
            M30RND = 295,    ///< Macro 30 Randomness

            N1AMT = 296,     ///< Envelope 1 Amount
            N1INI = 297,     ///< Envelope 1 Initial Level
            N1DEL = 298,     ///< Envelope 1 Delay Time
            N1ATK = 299,     ///< Envelope 1 Attack Time
            N1PK = 300,      ///< Envelope 1 Peak Level
            N1HLD = 301,     ///< Envelope 1 Hold Time
            N1DEC = 302,     ///< Envelope 1 Decay Time
            N1SUS = 303,     ///< Envelope 1 Sustain Level
            N1REL = 304,     ///< Envelope 1 Release Time
            N1FIN = 305,     ///< Envelope 1 Final Level
            N1TIN = 306,     ///< Envelope 1 Time Inaccuracy
            N1VIN = 307,     ///< Envelope 1 Level Inaccuracy

            N2AMT = 308,     ///< Envelope 2 Amount
            N2INI = 309,     ///< Envelope 2 Initial Level
            N2DEL = 310,     ///< Envelope 2 Delay Time
            N2ATK = 311,     ///< Envelope 2 Attack Time
            N2PK = 312,      ///< Envelope 2 Peak Level
            N2HLD = 313,     ///< Envelope 2 Hold Time
            N2DEC = 314,     ///< Envelope 2 Decay Time
            N2SUS = 315,     ///< Envelope 2 Sustain Level
            N2REL = 316,     ///< Envelope 2 Release Time
            N2FIN = 317,     ///< Envelope 2 Final Level
            N2TIN = 318,     ///< Envelope 2 Time Inaccuracy
            N2VIN = 319,     ///< Envelope 2 Level Inaccuracy

            N3AMT = 320,     ///< Envelope 3 Amount
            N3INI = 321,     ///< Envelope 3 Initial Level
            N3DEL = 322,     ///< Envelope 3 Delay Time
            N3ATK = 323,     ///< Envelope 3 Attack Time
            N3PK = 324,      ///< Envelope 3 Peak Level
            N3HLD = 325,     ///< Envelope 3 Hold Time
            N3DEC = 326,     ///< Envelope 3 Decay Time
            N3SUS = 327,     ///< Envelope 3 Sustain Level
            N3REL = 328,     ///< Envelope 3 Release Time
            N3FIN = 329,     ///< Envelope 3 Final Level
            N3TIN = 330,     ///< Envelope 3 Time Inaccuracy
            N3VIN = 331,     ///< Envelope 3 Level Inaccuracy

            N4AMT = 332,     ///< Envelope 4 Amount
            N4INI = 333,     ///< Envelope 4 Initial Level
            N4DEL = 334,     ///< Envelope 4 Delay Time
            N4ATK = 335,     ///< Envelope 4 Attack Time
            N4PK = 336,      ///< Envelope 4 Peak Level
            N4HLD = 337,     ///< Envelope 4 Hold Time
            N4DEC = 338,     ///< Envelope 4 Decay Time
            N4SUS = 339,     ///< Envelope 4 Sustain Level
            N4REL = 340,     ///< Envelope 4 Release Time
            N4FIN = 341,     ///< Envelope 4 Final Level
            N4TIN = 342,     ///< Envelope 4 Time Inaccuracy
            N4VIN = 343,     ///< Envelope 4 Level Inaccuracy

            N5AMT = 344,     ///< Envelope 5 Amount
            N5INI = 345,     ///< Envelope 5 Initial Level
            N5DEL = 346,     ///< Envelope 5 Delay Time
            N5ATK = 347,     ///< Envelope 5 Attack Time
            N5PK = 348,      ///< Envelope 5 Peak Level
            N5HLD = 349,     ///< Envelope 5 Hold Time
            N5DEC = 350,     ///< Envelope 5 Decay Time
            N5SUS = 351,     ///< Envelope 5 Sustain Level
            N5REL = 352,     ///< Envelope 5 Release Time
            N5FIN = 353,     ///< Envelope 5 Final Level
            N5TIN = 354,     ///< Envelope 5 Time Inaccuracy
            N5VIN = 355,     ///< Envelope 5 Level Inaccuracy

            N6AMT = 356,     ///< Envelope 6 Amount
            N6INI = 357,     ///< Envelope 6 Initial Level
            N6DEL = 358,     ///< Envelope 6 Delay Time
            N6ATK = 359,     ///< Envelope 6 Attack Time
            N6PK = 360,      ///< Envelope 6 Peak Level
            N6HLD = 361,     ///< Envelope 6 Hold Time
            N6DEC = 362,     ///< Envelope 6 Decay Time
            N6SUS = 363,     ///< Envelope 6 Sustain Level
            N6REL = 364,     ///< Envelope 6 Release Time
            N6FIN = 365,     ///< Envelope 6 Final Level
            N6TIN = 366,     ///< Envelope 6 Time Inaccuracy
            N6VIN = 367,     ///< Envelope 6 Level Inaccuracy

            N7AMT = 368,     ///< Envelope 7 Amount
            N7INI = 369,     ///< Envelope 7 Initial Level
            N7DEL = 370,     ///< Envelope 7 Delay Time
            N7ATK = 371,     ///< Envelope 7 Attack Time
            N7PK = 372,      ///< Envelope 7 Peak Level
            N7HLD = 373,     ///< Envelope 7 Hold Time
            N7DEC = 374,     ///< Envelope 7 Decay Time
            N7SUS = 375,     ///< Envelope 7 Sustain Level
            N7REL = 376,     ///< Envelope 7 Release Time
            N7FIN = 377,     ///< Envelope 7 Final Level
            N7TIN = 378,     ///< Envelope 7 Time Inaccuracy
            N7VIN = 379,     ///< Envelope 7 Level Inaccuracy

            N8AMT = 380,     ///< Envelope 8 Amount
            N8INI = 381,     ///< Envelope 8 Initial Level
            N8DEL = 382,     ///< Envelope 8 Delay Time
            N8ATK = 383,     ///< Envelope 8 Attack Time
            N8PK = 384,      ///< Envelope 8 Peak Level
            N8HLD = 385,     ///< Envelope 8 Hold Time
            N8DEC = 386,     ///< Envelope 8 Decay Time
            N8SUS = 387,     ///< Envelope 8 Sustain Level
            N8REL = 388,     ///< Envelope 8 Release Time
            N8FIN = 389,     ///< Envelope 8 Final Level
            N8TIN = 390,     ///< Envelope 8 Time Inaccuracy
            N8VIN = 391,     ///< Envelope 8 Level Inaccuracy

            N9AMT = 392,     ///< Envelope 9 Amount
            N9INI = 393,     ///< Envelope 9 Initial Level
            N9DEL = 394,     ///< Envelope 9 Delay Time
            N9ATK = 395,     ///< Envelope 9 Attack Time
            N9PK = 396,      ///< Envelope 9 Peak Level
            N9HLD = 397,     ///< Envelope 9 Hold Time
            N9DEC = 398,     ///< Envelope 9 Decay Time
            N9SUS = 399,     ///< Envelope 9 Sustain Level
            N9REL = 400,     ///< Envelope 9 Release Time
            N9FIN = 401,     ///< Envelope 9 Final Level
            N9TIN = 402,     ///< Envelope 9 Time Inaccuracy
            N9VIN = 403,     ///< Envelope 9 Level Inaccuracy

            N10AMT = 404,    ///< Envelope 10 Amount
            N10INI = 405,    ///< Envelope 10 Initial Level
            N10DEL = 406,    ///< Envelope 10 Delay Time
            N10ATK = 407,    ///< Envelope 10 Attack Time
            N10PK = 408,     ///< Envelope 10 Peak Level
            N10HLD = 409,    ///< Envelope 10 Hold Time
            N10DEC = 410,    ///< Envelope 10 Decay Time
            N10SUS = 411,    ///< Envelope 10 Sustain Level
            N10REL = 412,    ///< Envelope 10 Release Time
            N10FIN = 413,    ///< Envelope 10 Final Level
            N10TIN = 414,    ///< Envelope 10 Time Inaccuracy
            N10VIN = 415,    ///< Envelope 10 Level Inaccuracy

            N11AMT = 416,    ///< Envelope 11 Amount
            N11INI = 417,    ///< Envelope 11 Initial Level
            N11DEL = 418,    ///< Envelope 11 Delay Time
            N11ATK = 419,    ///< Envelope 11 Attack Time
            N11PK = 420,     ///< Envelope 11 Peak Level
            N11HLD = 421,    ///< Envelope 11 Hold Time
            N11DEC = 422,    ///< Envelope 11 Decay Time
            N11SUS = 423,    ///< Envelope 11 Sustain Level
            N11REL = 424,    ///< Envelope 11 Release Time
            N11FIN = 425,    ///< Envelope 11 Final Level
            N11TIN = 426,    ///< Envelope 11 Time Inaccuracy
            N11VIN = 427,    ///< Envelope 11 Level Inaccuracy

            N12AMT = 428,    ///< Envelope 12 Amount
            N12INI = 429,    ///< Envelope 12 Initial Level
            N12DEL = 430,    ///< Envelope 12 Delay Time
            N12ATK = 431,    ///< Envelope 12 Attack Time
            N12PK = 432,     ///< Envelope 12 Peak Level
            N12HLD = 433,    ///< Envelope 12 Hold Time
            N12DEC = 434,    ///< Envelope 12 Decay Time
            N12SUS = 435,    ///< Envelope 12 Sustain Level
            N12REL = 436,    ///< Envelope 12 Release Time
            N12FIN = 437,    ///< Envelope 12 Final Level
            N12TIN = 438,    ///< Envelope 12 Time Inaccuracy
            N12VIN = 439,    ///< Envelope 12 Level Inaccuracy

            L1FRQ = 440,     ///< LFO 1 Frequency
            L1PHS = 441,     ///< LFO 1 Phase
            L1MIN = 442,     ///< LFO 1 Minimum Value
            L1MAX = 443,     ///< LFO 1 Maximum Value
            L1AMT = 444,     ///< LFO 1 Amount
            L1DST = 445,     ///< LFO 1 Distortion
            L1RND = 446,     ///< LFO 1 Randomness

            L2FRQ = 447,     ///< LFO 2 Frequency
            L2PHS = 448,     ///< LFO 2 Phase
            L2MIN = 449,     ///< LFO 2 Minimum Value
            L2MAX = 450,     ///< LFO 2 Maximum Value
            L2AMT = 451,     ///< LFO 2 Amount
            L2DST = 452,     ///< LFO 2 Distortion
            L2RND = 453,     ///< LFO 2 Randomness

            L3FRQ = 454,     ///< LFO 3 Frequency
            L3PHS = 455,     ///< LFO 3 Phase
            L3MIN = 456,     ///< LFO 3 Minimum Value
            L3MAX = 457,     ///< LFO 3 Maximum Value
            L3AMT = 458,     ///< LFO 3 Amount
            L3DST = 459,     ///< LFO 3 Distortion
            L3RND = 460,     ///< LFO 3 Randomness

            L4FRQ = 461,     ///< LFO 4 Frequency
            L4PHS = 462,     ///< LFO 4 Phase
            L4MIN = 463,     ///< LFO 4 Minimum Value
            L4MAX = 464,     ///< LFO 4 Maximum Value
            L4AMT = 465,     ///< LFO 4 Amount
            L4DST = 466,     ///< LFO 4 Distortion
            L4RND = 467,     ///< LFO 4 Randomness

            L5FRQ = 468,     ///< LFO 5 Frequency
            L5PHS = 469,     ///< LFO 5 Phase
            L5MIN = 470,     ///< LFO 5 Minimum Value
            L5MAX = 471,     ///< LFO 5 Maximum Value
            L5AMT = 472,     ///< LFO 5 Amount
            L5DST = 473,     ///< LFO 5 Distortion
            L5RND = 474,     ///< LFO 5 Randomness

            L6FRQ = 475,     ///< LFO 6 Frequency
            L6PHS = 476,     ///< LFO 6 Phase
            L6MIN = 477,     ///< LFO 6 Minimum Value
            L6MAX = 478,     ///< LFO 6 Maximum Value
            L6AMT = 479,     ///< LFO 6 Amount
            L6DST = 480,     ///< LFO 6 Distortion
            L6RND = 481,     ///< LFO 6 Randomness

            L7FRQ = 482,     ///< LFO 7 Frequency
            L7PHS = 483,     ///< LFO 7 Phase
            L7MIN = 484,     ///< LFO 7 Minimum Value
            L7MAX = 485,     ///< LFO 7 Maximum Value
            L7AMT = 486,     ///< LFO 7 Amount
            L7DST = 487,     ///< LFO 7 Distortion
            L7RND = 488,     ///< LFO 7 Randomness

            L8FRQ = 489,     ///< LFO 8 Frequency
            L8PHS = 490,     ///< LFO 8 Phase
            L8MIN = 491,     ///< LFO 8 Minimum Value
            L8MAX = 492,     ///< LFO 8 Maximum Value
            L8AMT = 493,     ///< LFO 8 Amount
            L8DST = 494,     ///< LFO 8 Distortion
            L8RND = 495,     ///< LFO 8 Randomness

            MODE = 496,      ///< Mode
            MWAV = 497,      ///< Modulator Waveform
            CWAV = 498,      ///< Carrier Waveform
            MF1TYP = 499,    ///< Modulator Filter 1 Type
            MF2TYP = 500,    ///< Modulator Filter 2 Type
            CF1TYP = 501,    ///< Carrier Filter 1 Type
            CF2TYP = 502,    ///< Carrier Filter 2 Type
            EF1TYP = 503,    ///< Effects Filter 1 Type
            EF2TYP = 504,    ///< Effects Filter 2 Type
            L1WAV = 505,     ///< LFO 1 Waveform
            L2WAV = 506,     ///< LFO 2 Waveform
            L3WAV = 507,     ///< LFO 3 Waveform
            L4WAV = 508,     ///< LFO 4 Waveform
            L5WAV = 509,     ///< LFO 5 Waveform
            L6WAV = 510,     ///< LFO 6 Waveform
            L7WAV = 511,     ///< LFO 7 Waveform
            L8WAV = 512,     ///< LFO 8 Waveform
            L1LOG = 513,     ///< LFO 1 Logarithmic Frequency
            L2LOG = 514,     ///< LFO 2 Logarithmic Frequency
            L3LOG = 515,     ///< LFO 3 Logarithmic Frequency
            L4LOG = 516,     ///< LFO 4 Logarithmic Frequency
            L5LOG = 517,     ///< LFO 5 Logarithmic Frequency
            L6LOG = 518,     ///< LFO 6 Logarithmic Frequency
            L7LOG = 519,     ///< LFO 7 Logarithmic Frequency
            L8LOG = 520,     ///< LFO 8 Logarithmic Frequency
            L1CEN = 521,     ///< LFO 1 Center
            L2CEN = 522,     ///< LFO 2 Center
            L3CEN = 523,     ///< LFO 3 Center
            L4CEN = 524,     ///< LFO 4 Center
            L5CEN = 525,     ///< LFO 5 Center
            L6CEN = 526,     ///< LFO 6 Center
            L7CEN = 527,     ///< LFO 7 Center
            L8CEN = 528,     ///< LFO 8 Center
            L1SYN = 529,     ///< LFO 1 Tempo Synchronization
            L2SYN = 530,     ///< LFO 2 Tempo Synchronization
            L3SYN = 531,     ///< LFO 3 Tempo Synchronization
            L4SYN = 532,     ///< LFO 4 Tempo Synchronization
            L5SYN = 533,     ///< LFO 5 Tempo Synchronization
            L6SYN = 534,     ///< LFO 6 Tempo Synchronization
            L7SYN = 535,     ///< LFO 7 Tempo Synchronization
            L8SYN = 536,     ///< LFO 8 Tempo Synchronization
            ECSYN = 537,     ///< Effects Chorus Tempo Synchronization
            EESYN = 538,     ///< Effects Echo Tempo Synchronization
            MF1LOG = 539,    ///< Modulator Filter 1 Logarithmic Frequency
            MF2LOG = 540,    ///< Modulator Filter 2 Logarithmic Frequency
            CF1LOG = 541,    ///< Carrier Filter 1 Logarithmic Frequency
            CF2LOG = 542,    ///< Carrier Filter 2 Logarithmic Frequency
            EF1LOG = 543,    ///< Effects Filter 1 Logarithmic Frequency
            EF2LOG = 544,    ///< Effects Filter 2 Logarithmic Frequency
            ECLOG = 545,     ///< Effects Chorus Logarithmic Filter Frequencies
            ECLHQ = 546,     ///< Effects Chorus Logarithmic Highpass Filter Q Factor
            ECLLG = 547,     ///< Effects Chorus Logarithmic LFO Frequency
            EELOG = 548,     ///< Effects Echo Logarithmic Filter Frequencies
            EELHQ = 549,     ///< Effects Echo Logarithmic Highpass Filter Q Factor
            ERLOG = 550,     ///< Effects Reverb Logarithmic Filter Frequencies
            ERLHQ = 551,     ///< Effects Reverb Logarithmic Highpass Filter Q Factor
            N1UPD = 552,     ///< Envelope 1 Update Mode
            N2UPD = 553,     ///< Envelope 2 Update Mode
            N3UPD = 554,     ///< Envelope 3 Update Mode
            N4UPD = 555,     ///< Envelope 4 Update Mode
            N5UPD = 556,     ///< Envelope 5 Update Mode
            N6UPD = 557,     ///< Envelope 6 Update Mode
            N7UPD = 558,     ///< Envelope 7 Update Mode
            N8UPD = 559,     ///< Envelope 8 Update Mode
            N9UPD = 560,     ///< Envelope 9 Update Mode
            N10UPD = 561,    ///< Envelope 10 Update Mode
            N11UPD = 562,    ///< Envelope 11 Update Mode
            N12UPD = 563,    ///< Envelope 12 Update Mode
            POLY = 564,      ///< Polyphonic
            ERTYP = 565,     ///< Effects Reverb Type
            ECTYP = 566,     ///< Effects Chorus Type
            MTUN = 567,      ///< Modulator Tuning
            CTUN = 568,      ///< Carrier Tuning
            MOIA = 569,      ///< Modulator Oscillator Inaccuracy
            MOIS = 570,      ///< Modulator Oscillator Instability
            COIA = 571,      ///< Carrier Oscillator Inaccuracy
            COIS = 572,      ///< Carrier Oscillator Instability
            MF1QLG = 573,    ///< Modulator Filter 1 Logarithmic Q Factor
            MF2QLG = 574,    ///< Modulator Filter 2 Logarithmic Q Factor
            CF1QLG = 575,    ///< Carrier Filter 1 Logarithmic Q Factor
            CF2QLG = 576,    ///< Carrier Filter 2 Logarithmic Q Factor
            EF1QLG = 577,    ///< Effects Filter 1 Logarithmic Q Factor
            EF2QLG = 578,    ///< Effects Filter 2 Logarithmic Q Factor
            L1AEN = 579,     ///< LFO 1 Amount Envelope
            L2AEN = 580,     ///< LFO 2 Amount Envelope
            L3AEN = 581,     ///< LFO 3 Amount Envelope
            L4AEN = 582,     ///< LFO 4 Amount Envelope
            L5AEN = 583,     ///< LFO 5 Amount Envelope
            L6AEN = 584,     ///< LFO 6 Amount Envelope
            L7AEN = 585,     ///< LFO 7 Amount Envelope
            L8AEN = 586,     ///< LFO 8 Amount Envelope
            N1SYN = 587,     ///< Envelope 1 Tempo Synchronization
            N2SYN = 588,     ///< Envelope 2 Tempo Synchronization
            N3SYN = 589,     ///< Envelope 3 Tempo Synchronization
            N4SYN = 590,     ///< Envelope 4 Tempo Synchronization
            N5SYN = 591,     ///< Envelope 5 Tempo Synchronization
            N6SYN = 592,     ///< Envelope 6 Tempo Synchronization
            N7SYN = 593,     ///< Envelope 7 Tempo Synchronization
            N8SYN = 594,     ///< Envelope 8 Tempo Synchronization
            N9SYN = 595,     ///< Envelope 9 Tempo Synchronization
            N10SYN = 596,    ///< Envelope 10 Tempo Synchronization
            N11SYN = 597,    ///< Envelope 11 Tempo Synchronization
            N12SYN = 598,    ///< Envelope 12 Tempo Synchronization
            N1ASH = 599,     ///< Envelope 1 Attack Shape
            N2ASH = 600,     ///< Envelope 2 Attack Shape
            N3ASH = 601,     ///< Envelope 3 Attack Shape
            N4ASH = 602,     ///< Envelope 4 Attack Shape
            N5ASH = 603,     ///< Envelope 5 Attack Shape
            N6ASH = 604,     ///< Envelope 6 Attack Shape
            N7ASH = 605,     ///< Envelope 7 Attack Shape
            N8ASH = 606,     ///< Envelope 8 Attack Shape
            N9ASH = 607,     ///< Envelope 9 Attack Shape
            N10ASH = 608,    ///< Envelope 10 Attack Shape
            N11ASH = 609,    ///< Envelope 11 Attack Shape
            N12ASH = 610,    ///< Envelope 12 Attack Shape
            N1DSH = 611,     ///< Envelope 1 Decay Shape
            N2DSH = 612,     ///< Envelope 2 Decay Shape
            N3DSH = 613,     ///< Envelope 3 Decay Shape
            N4DSH = 614,     ///< Envelope 4 Decay Shape
            N5DSH = 615,     ///< Envelope 5 Decay Shape
            N6DSH = 616,     ///< Envelope 6 Decay Shape
            N7DSH = 617,     ///< Envelope 7 Decay Shape
            N8DSH = 618,     ///< Envelope 8 Decay Shape
            N9DSH = 619,     ///< Envelope 9 Decay Shape
            N10DSH = 620,    ///< Envelope 10 Decay Shape
            N11DSH = 621,    ///< Envelope 11 Decay Shape
            N12DSH = 622,    ///< Envelope 12 Decay Shape
            N1RSH = 623,     ///< Envelope 1 Release Shape
            N2RSH = 624,     ///< Envelope 2 Release Shape
            N3RSH = 625,     ///< Envelope 3 Release Shape
            N4RSH = 626,     ///< Envelope 4 Release Shape
            N5RSH = 627,     ///< Envelope 5 Release Shape
            N6RSH = 628,     ///< Envelope 6 Release Shape
            N7RSH = 629,     ///< Envelope 7 Release Shape
            N8RSH = 630,     ///< Envelope 8 Release Shape
            N9RSH = 631,     ///< Envelope 9 Release Shape
            N10RSH = 632,    ///< Envelope 10 Release Shape
            N11RSH = 633,    ///< Envelope 11 Release Shape
            N12RSH = 634,    ///< Envelope 12 Release Shape
            M1DSH = 635,     ///< Macro 1 Distortion Shape
            M2DSH = 636,     ///< Macro 2 Distortion Shape
            M3DSH = 637,     ///< Macro 3 Distortion Shape
            M4DSH = 638,     ///< Macro 4 Distortion Shape
            M5DSH = 639,     ///< Macro 5 Distortion Shape
            M6DSH = 640,     ///< Macro 6 Distortion Shape
            M7DSH = 641,     ///< Macro 7 Distortion Shape
            M8DSH = 642,     ///< Macro 8 Distortion Shape
            M9DSH = 643,     ///< Macro 9 Distortion Shape
            M10DSH = 644,    ///< Macro 10 Distortion Shape
            M11DSH = 645,    ///< Macro 11 Distortion Shape
            M12DSH = 646,    ///< Macro 12 Distortion Shape
            M13DSH = 647,    ///< Macro 13 Distortion Shape
            M14DSH = 648,    ///< Macro 14 Distortion Shape
            M15DSH = 649,    ///< Macro 15 Distortion Shape
            M16DSH = 650,    ///< Macro 16 Distortion Shape
            M17DSH = 651,    ///< Macro 17 Distortion Shape
            M18DSH = 652,    ///< Macro 18 Distortion Shape
            M19DSH = 653,    ///< Macro 19 Distortion Shape
            M20DSH = 654,    ///< Macro 20 Distortion Shape
            M21DSH = 655,    ///< Macro 21 Distortion Shape
            M22DSH = 656,    ///< Macro 22 Distortion Shape
            M23DSH = 657,    ///< Macro 23 Distortion Shape
            M24DSH = 658,    ///< Macro 24 Distortion Shape
            M25DSH = 659,    ///< Macro 25 Distortion Shape
            M26DSH = 660,    ///< Macro 26 Distortion Shape
            M27DSH = 661,    ///< Macro 27 Distortion Shape
            M28DSH = 662,    ///< Macro 28 Distortion Shape
            M29DSH = 663,    ///< Macro 29 Distortion Shape
            M30DSH = 664,    ///< Macro 30 Distortion Shape

            PARAM_ID_COUNT = 665,
            INVALID_PARAM_ID = PARAM_ID_COUNT,
        };

        static constexpr Integer FLOAT_PARAMS = ParamId::MODE;

        enum ControllerId {
            NONE =                      Midi::NONE,                 ///< None
            MODULATION_WHEEL =          Midi::MODULATION_WHEEL,     ///< Modulation Wheel (CC 1)
            BREATH =                    Midi::BREATH,               ///< Breath (CC 2)
            UNDEFINED_1 =               Midi::UNDEFINED_1,          ///< Undefined (CC 3)
            FOOT_PEDAL =                Midi::FOOT_PEDAL,           ///< Foot Pedal (CC 4)
            PORTAMENTO_TIME =           Midi::PORTAMENTO_TIME,      ///< Portamento Time (CC 5)
            DATA_ENTRY =                Midi::DATA_ENTRY,           ///< Data Entry (CC 6)
            VOLUME =                    Midi::VOLUME,               ///< Volume (CC 7)
            BALANCE =                   Midi::BALANCE,              ///< Balance (CC 8)
            UNDEFINED_2 =               Midi::UNDEFINED_2,          ///< Undefined (CC 9)
            PAN =                       Midi::PAN,                  ///< Pan (CC 10)
            EXPRESSION_PEDAL =          Midi::EXPRESSION_PEDAL,     ///< Expression Pedal (CC 11)
            FX_CTL_1 =                  Midi::FX_CTL_1,             ///< Effect Control 1 (CC 12)
            FX_CTL_2 =                  Midi::FX_CTL_2,             ///< Effect Control 2 (CC 13)
            UNDEFINED_3 =               Midi::UNDEFINED_3,          ///< Undefined (CC 14)
            UNDEFINED_4 =               Midi::UNDEFINED_4,          ///< Undefined (CC 15)
            GENERAL_1 =                 Midi::GENERAL_1,            ///< General 1 (CC 16)
            GENERAL_2 =                 Midi::GENERAL_2,            ///< General 2 (CC 17)
            GENERAL_3 =                 Midi::GENERAL_3,            ///< General 3 (CC 18)
            GENERAL_4 =                 Midi::GENERAL_4,            ///< General 4 (CC 19)
            UNDEFINED_5 =               Midi::UNDEFINED_5,          ///< Undefined (CC 20)
            UNDEFINED_6 =               Midi::UNDEFINED_6,          ///< Undefined (CC 21)
            UNDEFINED_7 =               Midi::UNDEFINED_7,          ///< Undefined (CC 22)
            UNDEFINED_8 =               Midi::UNDEFINED_8,          ///< Undefined (CC 23)
            UNDEFINED_9 =               Midi::UNDEFINED_9,          ///< Undefined (CC 24)
            UNDEFINED_10 =              Midi::UNDEFINED_10,         ///< Undefined (CC 25)
            UNDEFINED_11 =              Midi::UNDEFINED_11,         ///< Undefined (CC 26)
            UNDEFINED_12 =              Midi::UNDEFINED_12,         ///< Undefined (CC 27)
            UNDEFINED_13 =              Midi::UNDEFINED_13,         ///< Undefined (CC 28)
            UNDEFINED_14 =              Midi::UNDEFINED_14,         ///< Undefined (CC 29)
            UNDEFINED_15 =              Midi::UNDEFINED_15,         ///< Undefined (CC 30)
            UNDEFINED_16 =              Midi::UNDEFINED_16,         ///< Undefined (CC 31)
            SUSTAIN_PEDAL =             Midi::SUSTAIN_PEDAL,        ///< Sustain Pedal (CC 64)
            SOUND_1 =                   Midi::SOUND_1,              ///< Sound 1 (CC 70)
            SOUND_2 =                   Midi::SOUND_2,              ///< Sound 2 (CC 71)
            SOUND_3 =                   Midi::SOUND_3,              ///< Sound 3 (CC 72)
            SOUND_4 =                   Midi::SOUND_4,              ///< Sound 4 (CC 73)
            SOUND_5 =                   Midi::SOUND_5,              ///< Sound 5 (CC 74)
            SOUND_6 =                   Midi::SOUND_6,              ///< Sound 6 (CC 75)
            SOUND_7 =                   Midi::SOUND_7,              ///< Sound 7 (CC 76)
            SOUND_8 =                   Midi::SOUND_8,              ///< Sound 8 (CC 77)
            SOUND_9 =                   Midi::SOUND_9,              ///< Sound 9 (CC 78)
            SOUND_10 =                  Midi::SOUND_10,             ///< Sound 10 (CC 79)
            UNDEFINED_17 =              Midi::UNDEFINED_17,         ///< Undefined (CC 85)
            UNDEFINED_18 =              Midi::UNDEFINED_18,         ///< Undefined (CC 86)
            UNDEFINED_19 =              Midi::UNDEFINED_19,         ///< Undefined (CC 87)
            UNDEFINED_20 =              Midi::UNDEFINED_20,         ///< Undefined (CC 89)
            UNDEFINED_21 =              Midi::UNDEFINED_21,         ///< Undefined (CC 90)
            FX_1 =                      Midi::FX_1,                 ///< Effect 1 (CC 91)
            FX_2 =                      Midi::FX_2,                 ///< Effect 2 (CC 92)
            FX_3 =                      Midi::FX_3,                 ///< Effect 3 (CC 93)
            FX_4 =                      Midi::FX_4,                 ///< Effect 4 (CC 94)
            FX_5 =                      Midi::FX_5,                 ///< Effect 5 (CC 95)
            UNDEFINED_22 =              Midi::UNDEFINED_22,         ///< Undefined (CC 102)
            UNDEFINED_23 =              Midi::UNDEFINED_23,         ///< Undefined (CC 103)
            UNDEFINED_24 =              Midi::UNDEFINED_24,         ///< Undefined (CC 104)
            UNDEFINED_25 =              Midi::UNDEFINED_25,         ///< Undefined (CC 105)
            UNDEFINED_26 =              Midi::UNDEFINED_26,         ///< Undefined (CC 106)
            UNDEFINED_27 =              Midi::UNDEFINED_27,         ///< Undefined (CC 107)
            UNDEFINED_28 =              Midi::UNDEFINED_28,         ///< Undefined (CC 108)
            UNDEFINED_29 =              Midi::UNDEFINED_29,         ///< Undefined (CC 109)
            UNDEFINED_30 =              Midi::UNDEFINED_30,         ///< Undefined (CC 110)
            UNDEFINED_31 =              Midi::UNDEFINED_31,         ///< Undefined (CC 111)
            UNDEFINED_32 =              Midi::UNDEFINED_32,         ///< Undefined (CC 112)
            UNDEFINED_33 =              Midi::UNDEFINED_33,         ///< Undefined (CC 113)
            UNDEFINED_34 =              Midi::UNDEFINED_34,         ///< Undefined (CC 114)
            UNDEFINED_35 =              Midi::UNDEFINED_35,         ///< Undefined (CC 115)
            UNDEFINED_36 =              Midi::UNDEFINED_36,         ///< Undefined (CC 116)
            UNDEFINED_37 =              Midi::UNDEFINED_37,         ///< Undefined (CC 117)
            UNDEFINED_38 =              Midi::UNDEFINED_38,         ///< Undefined (CC 118)
            UNDEFINED_39 =              Midi::UNDEFINED_39,         ///< Undefined (CC 119)

            PITCH_WHEEL =               128,                        ///< Pitch Wheel

            TRIGGERED_NOTE =            129,                        ///< Triggered Note
            TRIGGERED_VELOCITY =        130,                        ///< Triggered Note's Velocity

            MACRO_1 =                   131,                        ///< Macro 1
            MACRO_2 =                   132,                        ///< Macro 2
            MACRO_3 =                   133,                        ///< Macro 3
            MACRO_4 =                   134,                        ///< Macro 4
            MACRO_5 =                   135,                        ///< Macro 5
            MACRO_6 =                   136,                        ///< Macro 6
            MACRO_7 =                   137,                        ///< Macro 7
            MACRO_8 =                   138,                        ///< Macro 8
            MACRO_9 =                   139,                        ///< Macro 9
            MACRO_10 =                  140,                        ///< Macro 10

            LFO_1 =                     141,                        ///< LFO 1
            LFO_2 =                     142,                        ///< LFO 2
            LFO_3 =                     143,                        ///< LFO 3
            LFO_4 =                     144,                        ///< LFO 4
            LFO_5 =                     145,                        ///< LFO 5
            LFO_6 =                     146,                        ///< LFO 6
            LFO_7 =                     147,                        ///< LFO 7
            LFO_8 =                     148,                        ///< LFO 8

            ENVELOPE_1 =                149,                        ///< Envelope 1
            ENVELOPE_2 =                150,                        ///< Envelope 2
            ENVELOPE_3 =                151,                        ///< Envelope 3
            ENVELOPE_4 =                152,                        ///< Envelope 4
            ENVELOPE_5 =                153,                        ///< Envelope 5
            ENVELOPE_6 =                154,                        ///< Envelope 6

            CHANNEL_PRESSURE =          155,                        ///< Channel Pressure

            MIDI_LEARN =                156,                        ///< MIDI Learn

            MACRO_11 =                  157,                        ///< Macro 11
            MACRO_12 =                  158,                        ///< Macro 12
            MACRO_13 =                  159,                        ///< Macro 13
            MACRO_14 =                  160,                        ///< Macro 14
            MACRO_15 =                  161,                        ///< Macro 15
            MACRO_16 =                  162,                        ///< Macro 16
            MACRO_17 =                  163,                        ///< Macro 17
            MACRO_18 =                  164,                        ///< Macro 18
            MACRO_19 =                  165,                        ///< Macro 19
            MACRO_20 =                  166,                        ///< Macro 20

            OSC_1_PEAK =                167,                        ///< Oscillator 1 Peak
            OSC_2_PEAK =                168,                        ///< Oscillator 1 Peak
            VOL_1_PEAK =                169,                        ///< Volume 1 Peak
            VOL_2_PEAK =                170,                        ///< Volume 2 Peak
            VOL_3_PEAK =                171,                        ///< Volume 3 Peak

            ENVELOPE_7 =                172,                        ///< Envelope 7
            ENVELOPE_8 =                173,                        ///< Envelope 8
            ENVELOPE_9 =                174,                        ///< Envelope 9
            ENVELOPE_10 =               175,                        ///< Envelope 10
            ENVELOPE_11 =               176,                        ///< Envelope 11
            ENVELOPE_12 =               177,                        ///< Envelope 12

            RELEASED_NOTE =             178,                        ///< Released Note
            RELEASED_VELOCITY =         179,                        ///< Released Note's Velocity

            MACRO_21 =                  180,                        ///< Macro 21
            MACRO_22 =                  181,                        ///< Macro 22
            MACRO_23 =                  182,                        ///< Macro 23
            MACRO_24 =                  183,                        ///< Macro 24
            MACRO_25 =                  184,                        ///< Macro 25
            MACRO_26 =                  185,                        ///< Macro 26
            MACRO_27 =                  186,                        ///< Macro 27
            MACRO_28 =                  187,                        ///< Macro 28
            MACRO_29 =                  188,                        ///< Macro 29
            MACRO_30 =                  189,                        ///< Macro 30

            CONTROLLER_ID_COUNT =       190,
            INVALID_CONTROLLER_ID =     CONTROLLER_ID_COUNT,
        };

        static constexpr Byte MODE_MIX_AND_MOD = 0;
        static constexpr Byte MODE_SPLIT_AT_C3 = 1;
        static constexpr Byte MODE_SPLIT_AT_Db3 = 2;
        static constexpr Byte MODE_SPLIT_AT_D3 = 3;
        static constexpr Byte MODE_SPLIT_AT_Eb3 = 4;
        static constexpr Byte MODE_SPLIT_AT_E3 = 5;
        static constexpr Byte MODE_SPLIT_AT_F3 = 6;
        static constexpr Byte MODE_SPLIT_AT_Gb3 = 7;
        static constexpr Byte MODE_SPLIT_AT_G3 = 8;
        static constexpr Byte MODE_SPLIT_AT_Ab3 = 9;
        static constexpr Byte MODE_SPLIT_AT_A3 = 10;
        static constexpr Byte MODE_SPLIT_AT_Bb3 = 11;
        static constexpr Byte MODE_SPLIT_AT_B3 = 12;
        static constexpr Byte MODE_SPLIT_AT_C4 = 13;

        static constexpr int MODES = 14;

        class Message
        {
            public:
                Message() noexcept;
                Message(Message const& message) noexcept = default;
                Message(Message&& message) noexcept = default;

                Message(
                    MessageType const type,
                    ParamId const param_id,
                    Number const number_param,
                    Byte const byte_param
                ) noexcept;

                Message& operator=(Message const& message) noexcept = default;
                Message& operator=(Message&& message) noexcept = default;

                MessageType type;
                ParamId param_id;
                Number number_param;
                Byte byte_param;
        };

        class ModeParam : public ByteParam
        {
            public:
                explicit ModeParam(std::string const& name) noexcept;
        };

        class NoteTuning
        {
            public:
                NoteTuning() noexcept
                    : frequency(0.0),
                    channel(Midi::INVALID_CHANNEL),
                    note(Midi::INVALID_NOTE)
                {
                }

                NoteTuning(NoteTuning const& note_tuning) noexcept = default;
                NoteTuning(NoteTuning&& note_tuning) noexcept = default;

                NoteTuning(
                    Midi::Channel const channel,
                    Midi::Note const note,
                    Frequency const frequency = 0.0
                ) noexcept
                    : frequency(frequency),
                    channel(channel),
                    note(note)
                {
                }

                NoteTuning& operator=(NoteTuning const& note_tuning) noexcept = default;
                NoteTuning& operator=(NoteTuning&& note_tuning) noexcept = default;

                bool is_valid() const noexcept
                {
                    return channel <= Midi::CHANNEL_MAX && note <= Midi::NOTE_MAX;
                }

                Frequency frequency;
                Midi::Channel channel;
                Midi::Note note;
        };

        typedef NoteTuning NoteTunings[POLYPHONY];

        static bool is_supported_midi_controller(
            Midi::Controller const controller
        ) noexcept;

        static bool is_controller_polyphonic(
            ControllerId const controller_id
        ) noexcept;

        static Number calculate_inaccuracy_seed(Integer const voice) noexcept;

        explicit Synth(Integer const samples_between_gc = 8000) noexcept;
        virtual ~Synth() override;

        virtual void set_sample_rate(Frequency const new_sample_rate) noexcept override;
        virtual void set_block_size(Integer const new_block_size) noexcept override;
        virtual void reset() noexcept override;

        bool is_lock_free() const noexcept;

        bool is_dirty() const noexcept;
        void clear_dirty_flag() noexcept;

        void suspend() noexcept;
        void resume() noexcept;

        bool has_mts_esp_tuning() const noexcept;
        bool has_continuous_mts_esp_tuning() const noexcept;
        bool is_mts_esp_connected() const noexcept;
        void mts_esp_connected() noexcept;
        void mts_esp_disconnected() noexcept;
        NoteTunings& collect_active_notes(Integer& active_notes_count) noexcept;
        void update_note_tuning(NoteTuning const& note_tuning) noexcept;
        void update_note_tunings(NoteTunings const& note_tunings, Integer const count) noexcept;

        Sample const* const* generate_samples(
            Integer const round, Integer const sample_count
        ) noexcept;

        /**
         * \brief Thread-safe way to change the state of the synthesizer outside
         *        the audio thread.
         */
        void push_message(
            MessageType const type,
            ParamId const param_id,
            Number const number_param,
            Byte const byte_param
        ) noexcept;

        /**
         * \brief Thread-safe way to change the state of the synthesizer outside
         *        the audio thread.
         */
        void push_message(Message const& message) noexcept;

        void process_messages() noexcept;

        /**
         * \brief Process a state changing message inside the audio thread.
         */
        void process_message(
            MessageType const type,
            ParamId const param_id,
            Number const number_param,
            Byte const byte_param
        ) noexcept;

        /**
         * \brief Process a state changing message inside the audio thread.
         */
        void process_message(Message const& message) noexcept;

        std::string const& get_param_name(ParamId const param_id) const noexcept;
        ParamId get_param_id(std::string const& name) const noexcept;

#ifdef JS80P_ASSERTIONS
        void get_param_id_hash_table_statistics(
            Integer& max_collisions,
            Number& avg_collisions,
            Number& avg_bucket_size
        ) const noexcept;
#endif

        Number float_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;

        Byte byte_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;

        bool is_toggle_param(ParamId const param_id) const noexcept;

        Number get_param_max_value(ParamId const param_id) const noexcept;
        Number get_param_ratio_atomic(ParamId const param_id) const noexcept;
        Number get_param_default_ratio(ParamId const param_id) const noexcept;

        ControllerId get_param_controller_id_atomic(
            ParamId const param_id
        ) const noexcept;

        void note_off(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Midi::Byte const velocity
        ) noexcept;

        void note_on(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Midi::Byte const velocity
        ) noexcept;

        void aftertouch(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Midi::Byte const pressure
        ) noexcept;

        void control_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Controller const controller,
            Midi::Byte const new_value
        ) noexcept;

        void channel_pressure(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Byte const pressure
        ) noexcept;

        void pitch_wheel_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Word const new_value
        ) noexcept;

        void all_sound_off(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void reset_all_controllers(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void all_notes_off(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void mono_mode_on(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void mono_mode_off(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        ToggleParam polyphonic;
        ModeParam mode;
        FloatParamS modulator_add_volume;
        FloatParamS phase_modulation_level;
        FloatParamS frequency_modulation_level;
        FloatParamS amplitude_modulation_level;

        Modulator::Params modulator_params;
        Carrier::Params carrier_params;

        MidiController pitch_wheel;
        MidiController triggered_note;
        MidiController released_note;
        MidiController triggered_velocity;
        MidiController released_velocity;
        MidiController channel_pressure_ctl;
        MidiController osc_1_peak;
        MidiController osc_2_peak;
        MidiController vol_1_peak;
        MidiController vol_2_peak;
        MidiController vol_3_peak;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        void finalize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        FrequencyTable frequencies;
        PerChannelFrequencyTable per_channel_frequencies;

    private:
        class Bus : public SignalProducer
        {
            friend class SignalProducer;

            public:
                Bus(
                    Integer const channels,
                    Modulator* const* const modulators,
                    Modulator::Params const& modulator_params,
                    Carrier* const* const carriers,
                    Carrier::Params const& carrier_params,
                    Integer const polyphony,
                    FloatParamS& modulator_add_volume
                ) noexcept;

                virtual ~Bus();

                virtual void set_block_size(
                    Integer const new_block_size
                ) noexcept override;

                void find_modulators_peak(
                    Integer const sample_count,
                    Sample& peak,
                    Integer& peak_index
                ) noexcept;

                void find_carriers_peak(
                    Integer const sample_count,
                    Sample& peak,
                    Integer& peak_index
                ) noexcept;

                void collect_active_notes(
                    NoteTunings& note_tunings,
                    Integer& note_tunings_count
                ) noexcept;

            protected:
                Sample const* const* initialize_rendering(
                    Integer const round,
                    Integer const sample_count
                ) noexcept;

                void render(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) noexcept;

            private:
                void allocate_buffers() noexcept;
                void free_buffers() noexcept;
                void reallocate_buffers() noexcept;

                void collect_active_voices() noexcept;

                template<class VoiceClass, bool should_sync_oscillator_inaccuracy, bool should_sync_oscillator_instability>
                void render_voices(
                    VoiceClass* (&voices)[POLYPHONY],
                    size_t const voices_count,
                    typename VoiceClass::Params const& params,
                    Integer const round,
                    Integer const sample_count
                ) noexcept;

                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index
                ) noexcept;

                template<bool is_additive_volume_constant>
                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample const add_volume_value,
                    Sample const* add_volume_buffer
                ) noexcept;

                void mix_carriers(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index
                ) noexcept;

                Integer const polyphony;
                Modulator* const* const modulators;
                Carrier* const* const carriers;
                Modulator::Params const& modulator_params;
                Carrier::Params const& carrier_params;
                Modulator* active_modulators[POLYPHONY];
                Carrier* active_carriers[POLYPHONY];
                size_t active_modulators_count;
                size_t active_carriers_count;
                FloatParamS& modulator_add_volume;
                Sample const* modulator_add_volume_buffer;
                Sample** modulators_buffer;
                Sample** carriers_buffer;
        };

        class ParamIdHashTable
        {
            public:
                ParamIdHashTable() noexcept;
                ~ParamIdHashTable();

                void add(std::string const& name, ParamId const param_id) noexcept;
                ParamId lookup(std::string const& name) noexcept;

#ifdef JS80P_ASSERTIONS
                void get_statistics(
                    Integer& max_collisions,
                    Number& avg_collisions,
                    Number& avg_bucket_size
                ) const noexcept;
#endif

            private:
                class Entry
                {
                    public:
                        static constexpr Integer NAME_SIZE = 8;
                        static constexpr Integer NAME_MAX_INDEX = NAME_SIZE - 1;

                        Entry() noexcept;
                        Entry(const char* name, ParamId const param_id) noexcept;
                        ~Entry();

                        void set(const char* name, ParamId const param_id) noexcept;

                        Entry *next;
                        char name[NAME_SIZE];
                        ParamId param_id;
                };

                static constexpr Integer ENTRIES = 0x100;
                static constexpr Integer MASK = ENTRIES - 1;
                static constexpr Integer MULTIPLIER = 257;
                static constexpr Integer SHIFT = 8;

                static Integer hash(std::string const& name) noexcept;

                void lookup(
                    std::string const& name,
                    Entry** root,
                    Entry** parent,
                    Entry** entry
                ) noexcept;

                Entry entries[ENTRIES];
        };

        class MidiControllerMessage
        {
            public:
                MidiControllerMessage();
                MidiControllerMessage(MidiControllerMessage const& message) = default;
                MidiControllerMessage(MidiControllerMessage&& message) = default;

                MidiControllerMessage(Seconds const time_offset, Midi::Word const value);

                bool operator==(MidiControllerMessage const& message) const noexcept;
                MidiControllerMessage& operator=(MidiControllerMessage const& message) noexcept = default;
                MidiControllerMessage& operator=(MidiControllerMessage&& message) noexcept = default;

            private:
                Seconds time_offset;
                Midi::Word value;
        };

        class DeferredNoteOff
        {
            public:
                DeferredNoteOff();
                DeferredNoteOff(DeferredNoteOff const& deferred_note_off) = default;
                DeferredNoteOff(DeferredNoteOff&& deferred_note_off) = default;

                DeferredNoteOff(
                    Integer const note_id,
                    Midi::Channel const channel,
                    Midi::Note const note,
                    Midi::Byte const velocity,
                    Integer const voice
                );

                DeferredNoteOff& operator=(DeferredNoteOff const& deferred_note_off) noexcept = default;
                DeferredNoteOff& operator=(DeferredNoteOff&& deferred_note_off) noexcept = default;

                Integer get_note_id() const noexcept;
                Midi::Channel get_channel() const noexcept;
                Midi::Note get_note() const noexcept;
                Midi::Byte get_velocity() const noexcept;
                Integer get_voice() const noexcept;

            private:
                Integer voice;
                Integer note_id;
                Midi::Channel channel;
                Midi::Note note;
                Midi::Byte velocity;
        };

        enum ParamType {
            OTHER = 0,
            SAMPLE_EVALUATED_FLOAT = 1,
            BLOCK_EVALUATED_FLOAT = 2,
            BYTE = 3,
            INVALID_PARAM_TYPE = 4,
        };

        static constexpr SPSCQueue<Message>::SizeType MESSAGE_QUEUE_SIZE = 8192;

        static constexpr Number MIDI_WORD_SCALE = 1.0 / 16384.0;
        static constexpr Number MIDI_BYTE_SCALE = 1.0 / 127.0;

        static constexpr Integer INVALID_VOICE = -1;

        static constexpr Integer NOTE_ID_MASK = 0x7fffffff;

        static constexpr Integer BIQUAD_FILTER_SHARED_BUFFERS = 6;

        static std::vector<bool> supported_midi_controllers;
        static bool supported_midi_controllers_initialized;

        static ParamIdHashTable param_id_hash_table;
        static std::string param_names_by_id[ParamId::PARAM_ID_COUNT];

        static bool should_sync_oscillator_inaccuracy(
            Modulator::Params const& modulator_params,
            Carrier::Params const& carrier_params
        ) noexcept;

        static bool should_sync_oscillator_instability(
            Modulator::Params const& modulator_params,
            Carrier::Params const& carrier_params
        ) noexcept;

        void initialize_supported_midi_controllers() noexcept;

        void build_frequency_table() noexcept;
        void register_main_params() noexcept;
        void register_modulator_params() noexcept;
        void register_carrier_params() noexcept;
        void register_effects_params() noexcept;
        void create_voices() noexcept;
        void create_midi_controllers() noexcept;
        void create_macros() noexcept;
        void create_envelopes() noexcept;
        void create_lfos() noexcept;

        void allocate_buffers() noexcept;
        void free_buffers() noexcept;
        void reallocate_buffers() noexcept;

        template<class ParamClass>
        void register_param_as_child(
            ParamId const param_id,
            ParamClass& param
        ) noexcept;

        template<class ParamClass>
        void register_param(ParamId const param_id, ParamClass& param) noexcept;

        ParamType find_param_type(ParamId const param_id) const noexcept;

        bool may_be_controllable(
            ParamId const param_id,
            ParamType const type
        ) const noexcept;

        Number midi_byte_to_float(Midi::Byte const midi_byte) const noexcept;
        Number midi_word_to_float(Midi::Word const midi_word) const noexcept;

        void sustain_on(Seconds const time_offset) noexcept;
        void sustain_off(Seconds const time_offset) noexcept;

        bool is_repeated_midi_controller_message(
            ControllerId const controller_id,
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Word const value
        ) noexcept;

        void stop_lfos() noexcept;
        void start_lfos() noexcept;

        void handle_set_param(
            ParamId const param_id,
            Number const ratio
        ) noexcept;

        void handle_assign_controller(
            ParamId const param_id,
            Byte const controller_id
        ) noexcept;

        void handle_refresh_param(ParamId const param_id) noexcept;

        void handle_clear() noexcept;

        bool assign_controller_to_byte_param(
            ParamId const param_id,
            ControllerId const controller_id
        ) noexcept;

        template<class FloatParamClass>
        bool assign_controller(
            FloatParamClass& param,
            ControllerId const controller_id
        ) noexcept;

        Number get_param_ratio(ParamId const param_id) const noexcept;

        void clear_midi_controllers() noexcept;

        void clear_midi_note_to_voice_assignments() noexcept;

        void clear_sustain() noexcept;

        bool should_sync_oscillator_inaccuracy() const noexcept;
        bool should_sync_oscillator_instability() const noexcept;

        void note_on_polyphonic(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        void note_on_monophonic(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity,
            bool const trigger_if_off
        ) noexcept;

        void trigger_note_on_voice(
            Integer const voice,
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        template<class VoiceClass>
        void trigger_note_on_voice_monophonic(
            VoiceClass& voice,
            bool const is_off,
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity,
            bool const should_sync_oscillator_inaccuracy
        ) noexcept;

        void assign_voice_and_note_id(
            Integer const voice,
            Midi::Channel const channel,
            Midi::Note const note
        ) noexcept;

        void stop_polyphonic_notes() noexcept;

        void update_param_states() noexcept;

        void garbage_collect_voices() noexcept;

        std::string const to_string(Integer const) const noexcept;

        std::vector<DeferredNoteOff> deferred_note_offs;
        SPSCQueue<Message> messages;
        Bus bus;
        NoteStack note_stack;
        PeakTracker osc_1_peak_tracker;
        PeakTracker osc_2_peak_tracker;
        PeakTracker vol_1_peak_tracker;
        PeakTracker vol_2_peak_tracker;
        PeakTracker vol_3_peak_tracker;

        Sample const* const* raw_output;
        MidiControllerMessage previous_controller_message[ControllerId::CONTROLLER_ID_COUNT];
        BiquadFilterSharedBuffers biquad_filter_shared_buffers[BIQUAD_FILTER_SHARED_BUFFERS];
        FloatParamS* sample_evaluated_float_params[ParamId::PARAM_ID_COUNT];
        FloatParamB* block_evaluated_float_params[ParamId::PARAM_ID_COUNT];
        ByteParam* byte_params[ParamId::PARAM_ID_COUNT];
        std::atomic<Number> param_ratios[ParamId::PARAM_ID_COUNT];
        std::atomic<Byte> controller_assignments[ParamId::PARAM_ID_COUNT];
        Envelope* envelopes_rw[Constants::ENVELOPES];
        LFO* lfos_rw[Constants::LFOS];
        Macro* macros_rw[MACROS];
        MidiController* midi_controllers_rw[MIDI_CONTROLLERS];
        Integer midi_note_to_voice_assignments[Midi::CHANNELS][Midi::NOTES];
        OscillatorInaccuracy* synced_oscillator_inaccuracies[POLYPHONY];
        Modulator* modulators[POLYPHONY];
        Carrier* carriers[POLYPHONY];
        NoteTunings active_note_tunings;
        Integer samples_since_gc;
        Integer samples_between_gc;
        Integer next_voice;
        Integer next_note_id;
        Midi::Note previous_note;
        bool is_learning;
        bool is_sustaining;
        bool is_polyphonic;
        bool was_polyphonic;
        bool is_dirty_;
        std::atomic<bool> is_mts_esp_connected_;

    public:
        Effects::Effects<Bus> effects;
        MidiController* const* const midi_controllers;
        Macro* const* const macros;
        Envelope* const* const envelopes;
        LFO* const* const lfos;
};

}

#endif
