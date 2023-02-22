/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
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

#ifndef JS80P__SYNTH__WAVETABLE_CPP
#define JS80P__SYNTH__WAVETABLE_CPP

#include <cmath>

#include "synth/wavetable.hpp"

#include "synth/math.hpp"


namespace JS80P
{

bool Wavetable::is_initialized = false;


Number Wavetable::sines[Wavetable::SIZE] = {0.0};


WavetableState::WavetableState()
{
}


void Wavetable::initialize()
{
    if (is_initialized) {
        return;
    }

    is_initialized = true;

    for (Integer j = 0; j != SIZE; ++j) {
        sines[j] = Math::sin(((Number)j * SIZE_INV) * Math::PI_DOUBLE);
    }
}


void Wavetable::reset_state(
        WavetableState& state,
        Seconds const sampling_period,
        Frequency const nyquist_frequency,
        Frequency const frequency,
        Seconds const start_time_offset
) {
    state.sample_index = (
        SIZE_FLOAT * (Number)start_time_offset * (Number)frequency
    );
    state.scale = SIZE_FLOAT * (Number)sampling_period;
    state.nyquist_frequency = nyquist_frequency;
    state.interpolation_limit = nyquist_frequency * INTERPOLATION_LIMIT_SCALE;
}


Wavetable::Wavetable(
        Number const coefficients[],
        Integer const coefficients_length
) : partials(coefficients_length)
{
    samples = new Sample*[partials];

    for (Integer i = 0; i != partials; ++i) {
        samples[i] = new Sample[SIZE];
    }

    update_coefficients(coefficients, true);
}


void Wavetable::update_coefficients(
        Number const coefficients[],
        bool const normalize
) {
    Integer frequency = 1;
    Sample max = 0.0;
    Sample sample;

    /*
    samples[0]: 0 partials above fundamental
    samples[1]: 1 partial above fundamental
    ...
    samples[n]: n partials above fundamental
    */

    for (Integer j = 0; j != SIZE; ++j) {
        sample = std::fabs(
            samples[0][j] = (
                (Sample)(coefficients[0] * sines[(j * frequency) & MASK])
            )
        );

        if (UNLIKELY(normalize && sample > max)) {
            max = sample;
        }
    }

    for (Integer i = 1; i != partials; ++i) {
        ++frequency;

        for (Integer j = 0; j != SIZE; ++j) {
            sample = std::fabs(
                samples[i][j] = (
                    samples[i - 1][j]
                    + (Sample)(coefficients[i] * sines[(j * frequency) & MASK])
                )
            );

            if (UNLIKELY(normalize && sample > max)) {
                max = sample;
            }
        }
    }

    if (UNLIKELY(normalize)) {
        for (Integer i = 0; i != partials; ++i) {
            for (Integer j = 0; j != SIZE; ++j) {
                samples[i][j] /= max;
            }
        }
    }
}


Wavetable::~Wavetable()
{
    for (Integer i = 0; i != partials; ++i) {
        delete[] samples[i];
    }

    delete[] samples;

    samples = NULL;
}


Sample Wavetable::lookup(WavetableState* state, Frequency const frequency) const
{
    Frequency const abs_frequency = std::fabs(frequency);

    if (UNLIKELY(abs_frequency < 0.0000001)) {
        return 1.0;
    }

    if (UNLIKELY(abs_frequency > state->nyquist_frequency)) {
        return 0.0;
    }

    Number const sample_index = state->sample_index;

    state->sample_index = wrap_around(
        sample_index + state->scale * (Number)frequency
    );

    Integer const partials = Wavetable::partials;

    if (partials == 1) {
        state->needs_table_interpolation = false;
        state->table_indices[0] = 0;

        return interpolate(state, abs_frequency, sample_index);
    }

    Sample const max_partials = (
        (Sample)(state->nyquist_frequency / abs_frequency)
    );
    Integer const more_partials_index = (
        std::max((Integer)0, std::min(partials, (Integer)max_partials) - 1)
    );
    Integer const fewer_partials_index = (
        std::max((Integer)0, more_partials_index - 1)
    );

    state->table_indices[0] = fewer_partials_index;

    if (more_partials_index == fewer_partials_index) {
        state->needs_table_interpolation = false;

        return interpolate(state, abs_frequency, sample_index);
    }

    state->needs_table_interpolation = true;
    state->table_indices[1] = more_partials_index;

    Sample const fewer_partials_weight = max_partials - std::floor(max_partials);
    Sample const more_partials_weight = 1.0 - fewer_partials_weight;

    state->table_weights[0] = fewer_partials_weight;
    state->table_weights[1] = more_partials_weight;

    return interpolate(state, abs_frequency, sample_index);
}


Number Wavetable::wrap_around(Number const index) const
{
    return index - std::floor(index * SIZE_INV) * SIZE_FLOAT;
}


Sample Wavetable::interpolate(
        WavetableState const* state,
        Frequency const frequency,
        Number const sample_index
) const {
    if (frequency >= state->interpolation_limit) {
        return interpolate_sample_linear(state, sample_index);
    } else {
        return interpolate_sample_lagrange(state, sample_index);
    }
}


Sample Wavetable::interpolate_sample_linear(
        WavetableState const* state,
        Number const sample_index
) const {
    Sample const sample_2_weight = (
        (Sample)(sample_index - std::floor(sample_index))
    );
    Sample const sample_1_weight = 1.0 - sample_2_weight;
    Integer const sample_1_index = (Integer)sample_index;
    Integer const sample_2_index = (sample_1_index + 1) & MASK;

    Sample const* table_1 = samples[state->table_indices[0]];

    if (!state->needs_table_interpolation) {
        return (
            sample_1_weight * table_1[sample_1_index]
            + sample_2_weight * table_1[sample_2_index]
        );
    }

    Sample const* table_2 = samples[state->table_indices[1]];

    return (
        state->table_weights[0] * (
            sample_1_weight * table_1[sample_1_index]
            + sample_2_weight * table_1[sample_2_index]
        )
        + state->table_weights[1] * (
            sample_1_weight * table_2[sample_1_index]
            + sample_2_weight * table_2[sample_2_index]
        )
    );
}


Sample Wavetable::interpolate_sample_lagrange(
        WavetableState const* state,
        Number const sample_index
) const {
    Integer const sample_1_index = (Integer)sample_index;
    Integer const sample_2_index = (sample_1_index + 1) & MASK;
    Integer const sample_3_index = (sample_2_index + 1) & MASK;

    Sample const* table_1 = samples[state->table_indices[0]];

    // Formula and notation from http://dlmf.nist.gov/3.3#ii

    Sample const f_1_1 = table_1[sample_1_index];
    Sample const f_1_2 = table_1[sample_2_index];
    Sample const f_1_3 = table_1[sample_3_index];

    Sample const t = (Sample)(sample_index - std::floor(sample_index));
    Sample const t_sqr = t * t;

    Sample const a_1 = 0.5 * (t_sqr - t);
    Sample const a_2 = 1.0 - t_sqr;
    Sample const a_3 = 0.5 * (t_sqr + t);

    if (!state->needs_table_interpolation) {
        return a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3;
    }

    Sample const* table_2 = samples[state->table_indices[1]];

    Sample const f_2_1 = table_2[sample_1_index];
    Sample const f_2_2 = table_2[sample_2_index];
    Sample const f_2_3 = table_2[sample_3_index];

    return (
        state->table_weights[0] * (a_1 * f_1_1 + a_2 * f_1_2 + a_3 * f_1_3)
        + state->table_weights[1] * (a_1 * f_2_1 + a_2 * f_2_2 + a_3 * f_2_3)
    );
}


StandardWavetables const StandardWavetables::standard_wavetables;


Wavetable const* StandardWavetables::sine()
{
    return standard_wavetables.sine_wt;
}


Wavetable const* StandardWavetables::sawtooth()
{
    return standard_wavetables.sawtooth_wt;
}


Wavetable const* StandardWavetables::inverse_sawtooth()
{
    return standard_wavetables.inverse_sawtooth_wt;
}


Wavetable const* StandardWavetables::triangle()
{
    return standard_wavetables.triangle_wt;
}


Wavetable const* StandardWavetables::square()
{
    return standard_wavetables.square_wt;
}


StandardWavetables::StandardWavetables()
{
    Wavetable::initialize();

    Number sine_coefficients[] = {1.0};
    Number sawtooth_coefficients[Wavetable::PARTIALS];
    Number inverse_sawtooth_coefficients[Wavetable::PARTIALS];
    Number triangle_coefficients[Wavetable::PARTIALS];
    Number square_coefficients[Wavetable::PARTIALS];

    for (Integer i = 0; i != Wavetable::PARTIALS; ++i) {
        Number const plus_or_minus_one = ((i & 1) == 1 ? -1.0 : 1.0);
        Number const i_pi = (Number)(i + 1) * Math::PI;
        Number const two_over_i_pi = 2.0 / i_pi;

        sawtooth_coefficients[i] = plus_or_minus_one * two_over_i_pi;
        inverse_sawtooth_coefficients[i] = -sawtooth_coefficients[i];
        triangle_coefficients[i] = (
            8.0 * Math::sin(i_pi / 2.0) / (i_pi * i_pi)
        );
        square_coefficients[i] = (1 + plus_or_minus_one) * two_over_i_pi;
    }

    sine_wt = new Wavetable(sine_coefficients, 1);
    sawtooth_wt = new Wavetable(sawtooth_coefficients, Wavetable::PARTIALS);
    inverse_sawtooth_wt = new Wavetable(
        inverse_sawtooth_coefficients, Wavetable::PARTIALS
    );
    triangle_wt = new Wavetable(triangle_coefficients, Wavetable::PARTIALS);
    square_wt = new Wavetable(square_coefficients, Wavetable::PARTIALS);
}


StandardWavetables::~StandardWavetables()
{
    delete sine_wt;
    delete sawtooth_wt;
    delete inverse_sawtooth_wt;
    delete triangle_wt;
    delete square_wt;

    sine_wt = NULL;
    sawtooth_wt = NULL;
    inverse_sawtooth_wt = NULL;
    triangle_wt = NULL;
    square_wt = NULL;
}

}

#endif