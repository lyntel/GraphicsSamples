//----------------------------------------------------------------------------------
// File:        es2-aurora\ParticleUpsampling/IceRevisitedRadix.h
// SDK Version: v3.00 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//----------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 *    Contains source code from the article "Radix Sort Revisited".
 *    \file        IceRevisitedRadix.h
 *    \author        Pierre Terdiman
 *    \date        April, 4, 2000
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Include Guard
#ifndef __ICERADIXSORT_H__
#define __ICERADIXSORT_H__

#include <NvSimpleTypes.h>


    typedef int8_t                sbyte;        //!<    sizeof(sbyte)    must be 1
    typedef uint8_t                ubyte;        //!<    sizeof(ubyte)    must be 1
    typedef int16_t                sword;        //!<    sizeof(sword)    must be 2
    typedef uint16_t               uword;        //!<    sizeof(uword)    must be 2
    typedef int32_t               sdword;        //!<    sizeof(sdword)    must be 4
    typedef uint32_t               udword;        //!<    sizeof(udword)    must be 4
    typedef float                float32;    //!<    sizeof(float32)    must be 4
    typedef double                float64;    //!<    sizeof(float64)    must be 4

    typedef udword                DynID;        //!<    Dynamic identifier
    typedef uword                KID;        //!<    Kernel ID
    typedef udword                RTYPE;        //!<    Relationship-type (!) between owners and references
    #define    INVALID_ID            0xffffffff    //!<    Invalid dword ID (counterpart of null pointers)
    #define    INVALID_KID            0xffff        //!<    Invalid Kernel ID
    #define    INVALID_NUMBER        0xDEADBEEF    //!<    Standard junk value

#define RADIX_LOCAL_RAM

#define DELETEARRAY(x)    { if (x != NULL) delete []x;    x = NULL; }
#define CHECKALLOC(x)    if(!x) return false;
#define PREVENT_COPY(curclass)    private: curclass(const curclass& object);    curclass& operator=(const curclass& object);

class RadixSort
{
    public:
    // Constructor/Destructor
                            RadixSort();
                            ~RadixSort();
    // Sorting methods
            RadixSort&        Sort(const udword* input, udword nb, bool signedvalues=true);
            RadixSort&        Sort(const float* input, udword nb);

    //! Access to results. mIndices is a list of indices in sorted order, i.e. in the order you may further process your data
    inline    udword*            GetIndices()        const    { return mIndices;        }

    //! mIndices2 gets trashed on calling the sort routine, but otherwise you can recycle it the way you want.
    inline    udword*            GetRecyclable()        const    { return mIndices2;        }

    // Stats
            udword            GetUsedRam()        const;
    //! Returns the total number of calls to the radix sorter.
    inline    udword            GetNbTotalCalls()    const    { return mTotalCalls;    }
    //! Returns the number of premature exits due to temporal coherence.
    inline    udword            GetNbHits()            const    { return mNbHits;        }

                            PREVENT_COPY(RadixSort)
    private:
#ifndef RADIX_LOCAL_RAM
            udword*            mHistogram;                    //!< Counters for each byte
            udword*            mOffset;                    //!< Offsets (nearly a cumulative distribution function)
#endif
            udword            mCurrentSize;                //!< Current size of the indices list
            udword            mPreviousSize;                //!< Size involved in previous call
            udword*            mIndices;                    //!< Two lists, swapped each pass
            udword*            mIndices2;
    // Stats
            udword            mTotalCalls;
            udword            mNbHits;
    // Internal methods
            bool            Resize(udword nb);
            void            ResetIndices();
};

#endif // __ICERADIXSORT_H__
