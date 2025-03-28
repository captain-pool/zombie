// This file defines a DomainSampler for generating uniformly distributed sample points
// in a 2D or 3D domain. These sample points are required by the Boundary Value Caching (BVC)
// and Reverse Walk Splatting (RWS) techniques for reducing variance of the walk-on-spheres
// and walk-on-stars estimators for PDEs with non-zero source.

#pragma once

#include <zombie/point_estimation/common.h>

namespace zombie {

template <typename T, size_t DIM>
class DomainSampler {
public:
    // generates sample points inside the user-specified solve region
    virtual void generateSamples(int nSamples, std::vector<SamplePoint<T, DIM>>& samplePts) = 0;
};

template <typename T, size_t DIM>
class UniformDomainSampler: public DomainSampler<T, DIM> {
public:
    // constructor
    UniformDomainSampler(const GeometricQueries<DIM>& queries_,
                         const std::function<bool(const Vector<DIM>&)>& insideSolveRegion_,
                         const Vector<DIM>& solveRegionMin_,
                         const Vector<DIM>& solveRegionMax_,
                         float solveRegionVolume_);

    // generates uniformly distributed sample points inside the solve region;
    // NOTE: may not generate exactly the requested number of samples when the
    // solve region volume does not match the volume of its bounding extents
    void generateSamples(int nSamples, std::vector<SamplePoint<T, DIM>>& samplePts);

protected:
    // members
    pcg32 sampler;
    const GeometricQueries<DIM>& queries;
    const std::function<bool(const Vector<DIM>&)>& insideSolveRegion;
    const Vector<DIM>& solveRegionMin;
    const Vector<DIM>& solveRegionMax;
    float solveRegionVolume;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
// FUTURE:
// - improve stratification, since it helps reduce clumping/singular artifacts
// - sample points in the domain in proportion to source values

template <typename T, size_t DIM>
inline UniformDomainSampler<T, DIM>::UniformDomainSampler(const GeometricQueries<DIM>& queries_,
                                                          const std::function<bool(const Vector<DIM>&)>& insideSolveRegion_,
                                                          const Vector<DIM>& solveRegionMin_,
                                                          const Vector<DIM>& solveRegionMax_,
                                                          float solveRegionVolume_):
                                                          queries(queries_),
                                                          insideSolveRegion(insideSolveRegion_),
                                                          solveRegionMin(solveRegionMin_),
                                                          solveRegionMax(solveRegionMax_),
                                                          solveRegionVolume(solveRegionVolume_)
{
    auto now = std::chrono::high_resolution_clock::now();
    uint64_t seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    sampler = pcg32(seed);
}

template <typename T, size_t DIM>
inline void UniformDomainSampler<T, DIM>::generateSamples(int nSamples, std::vector<SamplePoint<T, DIM>>& samplePts)
{
    // initialize sample points
    samplePts.clear();
    Vector<DIM> regionExtent = solveRegionMax - solveRegionMin;
    float pdf = 1.0f/solveRegionVolume;

    // generate stratified samples
    std::vector<float> stratifiedSamples;
    int nStratifiedSamples = nSamples;
    if (solveRegionVolume > 0.0f) nStratifiedSamples *= regionExtent.prod()*pdf;
    generateStratifiedSamples<DIM>(stratifiedSamples, nStratifiedSamples, sampler);

    // generate sample points inside the solve region
    for (int i = 0; i < nStratifiedSamples; i++) {
        Vector<DIM> randomVector = Vector<DIM>::Zero();
        for (int j = 0; j < DIM; j++) randomVector[j] = stratifiedSamples[DIM*i + j];
        Vector<DIM> pt = (solveRegionMin.array() + regionExtent.array()*randomVector.array()).matrix();

        if (insideSolveRegion(pt)) {
            float distToAbsorbingBoundary = queries.computeDistToAbsorbingBoundary(pt, false);
            float distToReflectingBoundary = queries.computeDistToReflectingBoundary(pt, false);
            SamplePoint<T, DIM> samplePt(pt, Vector<DIM>::Zero(), SampleType::InDomain,
                                         EstimationQuantity::Solution, pdf,
                                         distToAbsorbingBoundary, distToReflectingBoundary);
            samplePts.emplace_back(samplePt);
        }
    }
}

} // zombie
