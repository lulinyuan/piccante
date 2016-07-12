/*

PICCANTE
The hottest HDR imaging library!
http://vcg.isti.cnr.it/piccante

Copyright (C) 2014
Visual Computing Laboratory - ISTI CNR
http://vcg.isti.cnr.it
First author: Francesco Banterle

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PIC_ALGORITHMS_SUB_SAMPLE_STACK_HPP
#define PIC_ALGORITHMS_SUB_SAMPLE_STACK_HPP

#include "util/math.hpp"

#include "image.hpp"
#include "point_samplers/sampler_random.hpp"
#include "histogram.hpp"

namespace pic {

/**
 * @brief The SubSampleStack class
 */
class SubSampleStack
{
protected:

    /**
    * \brief This function creates a low resolution version of the stack using Grossberg and Nayar sampling.
    * \param stack is a stack of Image* at different exposures
    */
    void Grossberg(ImageVec &stack)
    {
        #ifdef PIC_DEBUG
            printf("Computing histograms...");
        #endif

        Histogram *h = new Histogram[exposures * channels];

        int c = 0;
        for(int j = 0; j < channels; j++) {
            for(unsigned int i = 0; i < exposures; i++) {
                h[c].Calculate(stack[i], VS_LDR, 256, j);
                h[c].cumulativef(true);
                c++;
            }
        }

        #ifdef PIC_DEBUG
            printf("Ok\n");
        #endif

        #ifdef PIC_DEBUG
            printf("Sampling...");
        #endif

        samples = new int[nSamples * channels * exposures];

        float div = float(nSamples - 1);
        c = 0;
        for(int k = 0; k < channels; k++) {
            for(int i = 0; i < nSamples; i++) {

                float u = float(i) / div;

                for(unsigned int j = 0; j < exposures; j++) {

                    int ind = k * exposures + j;

                    float *bin_c = h[ind].getCumulativef();

                    float *ptr = std::upper_bound(&bin_c[0], &bin_c[0]+256, u);

                    samples[c] = CLAMPi((int)(ptr - bin_c), 0, 255);
                    c++;
                }
            }
        }

        #ifdef PIC_DEBUG
            printf("Ok\n");
        #endif

        delete[] h;
    }

    /**
     * @brief Spatial creates a low resolution version of the stack.
     * @param stack is a stack of Image* at different exposures
     * @param sub_type
     */
    void Spatial(ImageVec &stack, SAMPLER_TYPE sub_type = ST_MONTECARLO_S)
    {
        int width    = stack[0]->width;
        int height   = stack[0]->height;

        Vec<2, int> vec(width, height);

        RandomSampler<2> *sampler = new RandomSampler<2>(sub_type, vec, nSamples, 1, 0);

        #ifdef PIC_DEBUG
            int oldNSamples = nSamples;
        #endif

        nSamples = sampler->getSamplesPerLevel(0);

        #ifdef PIC_DEBUG
            printf("--subSample samples: %d \t \t old samples: %d\n", nSamples, oldNSamples);
        #endif

        samples = new int[nSamples * channels * exposures];

        int c = 0;

        for(int k = 0; k < channels; k++) {
            for(int i = 0; i < nSamples; i++) {

                int x, y;
                sampler->getSampleAt(0, i, x, y);

                for(unsigned int j = 0; j < stack.size(); j++) {
                    float fetched = (*stack[j])(x, y)[k];
                    float tmp = lround(fetched * 255.0f);
                    samples[c] = CLAMPi(int(tmp), 0, 255);
                    c++;
                }
            }
        }

        delete sampler;
    }


    bool bCheck;

    unsigned int exposures;
    int channels;
    int nSamples;
    int total;
    int *samples;

public:
    
    /**
     * @brief SubSampleStack
     */
    SubSampleStack()
    {
        total = 0;
        exposures = 0;
        channels = 0;
        nSamples = 0;
        samples = NULL;
    }

    ~SubSampleStack()
    {
        Destroy();
    }

    void Destroy()
    {
        exposures = 0;
        channels = 0;
        nSamples = 0;
        total = 0;
        if(samples != NULL) {
            delete[] samples;
            samples = NULL;
        }
    }


    /**
     * @brief Compute
     * @param stack
     * @param nSamples output number of samples
     * @param bSpatial
     * @param sub_type
     */
    void Compute(ImageVec &stack, int nSamples, bool bRemoveOutliers, bool bSpatial = false, SAMPLER_TYPE sub_type = ST_MONTECARLO_S)
    {
        Destroy();

        bCheck = stack.size() > 1;
        bCheck = bCheck && (nSamples > 1);

        if(!bCheck) {
            return;
        }

        this->nSamples = nSamples;
        this->channels  = stack[0]->channels;
        this->exposures = stack.size();

        if(bSpatial) {
            Spatial(stack, sub_type);
        } else {
            Grossberg(stack);
        }

        if(bRemoveOutliers) {
            float t_min_f = 0.05f;
            float t_max_f = 1.0f - t_min_f;

            int t_min = int(t_min_f * 255.0f);
            int t_max = int(t_max_f * 255.0f);

            total = this->nSamples * this->channels * this->exposures;
            for(int i=0; i<total; i++) {
                if(samples[i] < t_min || samples[i] > t_max) {
                    samples[i] = -1;
                }
            }
        }
    }

    /**
     * @brief get
     * @return
     */
    int *get() {
        return samples;
    }

    int getNSamples() const {
        return nSamples;
    }

};

} // end namespace pic

#endif /* PIC_ALGORITHMS_SUB_SAMPLE_STACK_HPP */

