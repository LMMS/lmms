/*
 * eqfilter.cpp - defination of EqFilterclass.
 *
 * Copyright (c) 2014 David French <dave/dot/french3/at/googlemail/dot/com>
 *
 * This file is part of LMMS - http://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef EQFILTER_H
#define EQFILTER_H

#include "BasicFilters.h"
#include "lmms_math.h"

///
/// \brief The EqFilter class.
/// A wrapper for the StereoBiQuad class, giving it freq, res, and gain controls.
/// It is designed to process periods in one pass, with recalculation of coefficents
/// upon parameter changes. The intention is to use this as a bass class, children override
/// the calcCoefficents() function, providing the coefficents a1, a2, b0, b1, b2.
///
class EqFilter : public StereoBiQuad
{
public:
    EqFilter()
    {

    }

    virtual inline void setSampleRate( int sampleRate )
    {
        if( sampleRate != m_sampleRate )
        {
            m_sampleRate = sampleRate;
            calcCoefficents();
        }
    }

    virtual inline void setFrequency( float freq ){
        if ( freq != m_freq )
        {
            m_freq = freq;
            calcCoefficents();
        }
    }

    virtual void setQ( float res )
    {
        if ( res != m_res )
        {
            m_res = res;
            calcCoefficents();
        }
    }

    virtual void setGain( float gain )
    {
        if ( gain != m_gain )
        {
            m_gain = gain;
            calcCoefficents();
        }
    }

    ///
    /// \brief processBuffer
    /// \param buf Audio Buffer
    /// \param frames Count of sampleFrames in Audio Buffer
    ///
    virtual void processBuffer( sampleFrame* buf, const fpp_t frames )
    {
        for ( fpp_t f = 0 ; f < frames ; ++f)
        {
            buf[f][0] = update( buf[f][0] , 0);
            buf[f][1] = update( buf[f][1] , 1);
        }
    }

protected:
    ///
    /// \brief calcCoefficents
    ///  Override this in child classes to provide the coefficents, based on
    ///  Freq, Res and Gain
    virtual void calcCoefficents(){
        setCoeffs( 0, 0, 0, 0, 0 );

    }

    float m_sampleRate;
    float m_freq;
    float m_res;
    float m_gain;
};




///
/// \brief The EqHp12Filter class
/// A 2 pole High Pass Filter
/// Coefficent calculations from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
class EqHp12Filter : public EqFilter
{
public :
    virtual void calcCoefficents()
    {

        // calc intermediate
        float w0 = F_2PI * m_freq / m_sampleRate;
        float c = cosf( w0 );
        float s = sinf( w0 );
        float alpha = s / ( 2 * m_res );

        float a0, a1, a2, b0, b1, b2; // coeffs to calculate

        //calc coefficents
        b0 = ( 1 + c ) * 0.5;
        b1 = ( -( 1 + c ) );
        b2 = ( 1 + c ) * 0.5;
        a0 = 1 + alpha;
        a1 = ( -2 * c );
        a2 = 1 - alpha;

        //normalise
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;

        a0 = 1;

        setCoeffs( a1, a2, b0, b1, b2 );


    }
};


///
/// \brief The EqLp12Filter class.
/// A 2 pole low pass filter
/// Coefficent calculations from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
///
class EqLp12Filter : public EqFilter
{
public :
    virtual void calcCoefficents()
    {

        // calc intermediate
        float w0 = F_2PI * m_freq / m_sampleRate;
        float c = cosf( w0 );
        float s = sinf( w0 );
        float alpha = s / ( 2 * m_res );

        float a0, a1, a2, b0, b1, b2; // coeffs to calculate

        //calc coefficents
        b0 = ( 1 - c ) * 0.5;
        b1 = 1 - c;
        b2 = ( 1 - c ) * 0.5;
        a0 = 1 + alpha;
        a1 = -2 * c;
        a2 = 1 - alpha;

        //normalise
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;

        a0 = 1;

        setCoeffs( a1, a2, b0, b1, b2 );
    }
};



///
/// \brief The EqPeakFilter class
/// A Peak Filter
/// Coefficent calculations from http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt
///
class EqPeakFilter : public EqFilter
{
public:


    virtual void calcCoefficents()
    {
        // calc intermediate
        float w0 = F_2PI * m_freq / m_sampleRate;
        float c = cosf( w0 );
        float s = sinf( w0 );
        float A =  pow( 10, m_gain * 0.025);
        float alpha = s / ( 2 * m_res );

        float a0, a1, a2, b0, b1, b2; // coeffs to calculate

        //calc coefficents
        b0 =   1 + alpha*A;
        b1 =  -2*c;
        b2 =   1 - alpha*A;
        a0 =   1 + alpha/A;
        a1 =  -2*c;
        a2 =   1 - alpha/A;

        //normalise
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        a0 = 1;

        setCoeffs( a1, a2, b0, b1, b2 );
    }
};

class EqLowShelfFilter : public EqFilter
{
public :
    virtual void calcCoefficents()
    {

        // calc intermediate
        float w0 = F_2PI * m_freq / m_sampleRate;
        float c = cosf( w0 );
        float s = sinf( w0 );
        float A =  pow( 10, m_gain * 0.025);
//        float alpha = s / ( 2 * m_res );
        float beta = sqrt( A ) / m_res;

        float a0, a1, a2, b0, b1, b2; // coeffs to calculate

        //calc coefficents
        b0 = A * ( ( A+1 ) - ( A-1 ) * c + beta * s );
        b1 = 2  * A * ( ( A - 1 ) - ( A + 1 ) * c) ;
        b2 = A * ( ( A + 1 ) - ( A - 1 ) * c - beta * s);
        a0 = ( A + 1 ) + ( A - 1 ) * c + beta * s;
        a1 = -2 * ( ( A - 1 ) + ( A + 1 ) * c );
        a2 = ( A + 1 ) + ( A - 1) * c - beta * s;

        //normalise
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;

        a0 = 1;

        setCoeffs( a1, a2, b0, b1, b2 );


    }
};

class EqHighShelfFilter : public EqFilter
{
public :
    virtual void calcCoefficents()
    {

        // calc intermediate
        float w0 = F_2PI * m_freq / m_sampleRate;
        float c = cosf( w0 );
        float s = sinf( w0 );
        float A =  pow( 10, m_gain * 0.025 );
        float beta = sqrt( A ) / m_res;

        float a0, a1, a2, b0, b1, b2; // coeffs to calculate

        //calc coefficents
        b0 = A *( ( A +1 ) + ( A - 1 ) * c + beta * s);
        b1 = -2 * A * ( ( A - 1 ) + ( A + 1 ) * c );
        b2 = A * ( ( A + 1 ) + ( A - 1 ) * c - beta * s);
        a0 = ( A + 1 ) - ( A - 1 ) * c + beta * s;
        a1 = 2 * ( ( A - 1 ) - ( A + 1 ) * c );
        a2 = ( A + 1) - ( A - 1 ) * c - beta * s;
        //normalise
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        a0 = 1;

        setCoeffs( a1, a2, b0, b1, b2 );
    }
};




#endif // EQFILTER_H
