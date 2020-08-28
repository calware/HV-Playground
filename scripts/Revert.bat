@ECHO OFF
cd C:\Program Files (x86)\VMware\VMware Workstation
vmrun.exe revertToSnapshot "C:\Users\cal\Desktop\z\vms\w10_kdev_chk\Windows 10 x64 Kernel Debug [Checked]\Windows 10 x64 Kernel Debug [Checked].vmx" RocketDriver_v1_vmx_dbg
vmrun.exe start "C:\Users\cal\Desktop\z\vms\w10_kdev_chk\Windows 10 x64 Kernel Debug [Checked]\Windows 10 x64 Kernel Debug [Checked].vmx"