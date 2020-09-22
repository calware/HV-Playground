# GuestState Branch

This branch demonstrates an environment in which users can more comprehensively inspect and modify guest execution state. Additionally, it allows for continuous VM-entries via the `VMRESUME` instruction.

[**Compare this branch to it's base (master)**](https://github.com/calware/HV-Playground/compare/master...GuestState)

Several files have been added from the initial implementation. These files, along with a short description of their purpose, have been listed below.
 * **Guest(.h/c)** Houses the C code and definitions related to the guest execution environment. This includes assembly function definitions from `guestintrin.asm`, which are designed to be interoperable with C-level guest code. The C environment which facilitates guest execution is located in a function called `GuestEntry`, which can be optionally configured as the guest entry point in the call to `SetVMCSGuestState` in our `Driver.c` file.
 * **Handler(.h/c)** Outlines our previous `VMExitHandler` function from the `Driver.c` file to it's own source file (`Handler.c`), and defines two external functions, `RawHandler`, and `RestoreContext`, which are assembly functions found in the `rawhandler.asm` file.
   - `RawHandler` Enables our C VM-exit handler (`VMExitHandler`) to obtain guest execution state (instruction/stack pointers, GPRs, volatile registers, SSE controls, XMM registers, and FLAGS), as well as restoring this execution state, before finally facilitating VM-entries via `VMRESUME`.
   - `RestoreContext` Allows us to restore processor state based on `CONTEXT` structures (only supporting AMD64/x86_64 context structures), while bypassing many of the limitations of the NT function `RtlRestoreContext`, which our function is based off of—and was previously used in our VM-exit handler. See the comments within this function for more information on changes performed to the body of `RtlRestoreContext`
 * **Log(.h/c)** Supports logging via the [TraceLogging API](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/tracelogging-api), which is based on the [Event Tracing for Windows (ETW)](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/event-tracing-for-windows--etw-) platform. The advantage of using the TraceLogging API over our previous calls to `KdPrint` (`DbgPrint`) is that the TraceLogging API can be called at any IRQL, while KdPrint can only safely be called at an IRQL less than or equal to `DIRQL` \(see [DbgPrint function - MSDN](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-dbgprint)\). *Note: inside of our VM-exit handler, our VMM is [effectively running at a `HIGH_LEVEL` IRQL](https://github.com/tandasat/HyperPlatform/issues/3#issuecomment-230494046).*

Further, a helpful MASM developer manual found online has been added to the *research* directory.

Going forward, the next branch will pertain greatly to EPT (and the concepts therein)—for example *EPT splitting*, and other privilege-based processor behavior modifications. A branch will also be created which demonstrates event injection, in order to better simulate non-VMX operating environments while in VMX operation.
