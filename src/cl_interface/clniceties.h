#ifndef CLNICETIES_H
#define CLNICETIES_H

#include "include_opencl.h"
#include "myclimage.h"
#include "myclwrapper.h"

// TODO: Files should not depend on files higher up in the directory hierarchy.
#include "../utilitiesclprogram.h"

#include <map>

/*
    An interface to make some OpenCL-related procedures less tedious.
    The goals of the interface are:
        1) Easy to use efficiently.
        2) Readable code.
        3) Hard to misuse--one action should require only one function call.

    Tough question: how to handle contexts and devices...
        - Should this class have all methods accept a MyCLWrapper parameter?
            This would get tedious, since the same parameter would be passed every time.
        - Should this class have a method that must be called before usage that sets the MyCLWrapper parameter?
            This is error-prone and not enforceable by the compiler.
        - Should there be a global MyCLWrapper object?
            Most likely, a single application will only use one device and one context.
            Therefore, this makes sense.

    I select the third option. See MyCLWrapper.h class for thoughts about how to make it work.
*/
class CLNiceties
{
public:

    /// \brief Fills the image with zeros.
    ///
    /// Fills the image with zeros. This is done with an OpenCL program
    /// to avoid GPU->CPU->GPU copies. The program is run on the provided
    /// queue.
    ///
    /// \param queue        The command queue on which to run the zeroing program.
    /// \param zeroImage    The image to zero.
    /// \param wrapper      The device/context to use. If nullptr, uses the global context.
    static void ZeroImage(cl_command_queue queue, MyCLImage2D &zeroImage, MyCLWrapper *wrapper = nullptr);

    /// \brief Fills the image with zeros, assuming it is mapped.
    static void ZeroMappedImage(MyCLImage2D &zeroImage);

private:



    /// \brief Returns the utilities program associated to this wrapper,
    ///        creating it if necessary.
    UtilitiesCLProgram &utilitiesProgram(MyCLWrapper *wrapper);


    // This class can be used with multiple wrappers, and most
    // CL objects have to be bound to a particular context. PerCLWrapper
    // is used to keep track of wrapper-to-object mappings.
    template<typename T>
    using PerCLWrapper = std::map<MyCLWrapper *, T>;

    PerCLWrapper<UtilitiesCLProgram *> mUtilitiesPrograms;

    static CLNiceties &singleton();
    static CLNiceties *static_singleton;
};

#endif // CLNICETIES_H
