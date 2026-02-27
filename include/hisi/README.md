# HiSilicon SDK Headers

Place the Hi3516CV300 SDK header files here when building with the actual SDK.

Required headers (from `Hi3516CV300_SDK_V2.0.4.0/mpp/include/`):

- `hi_mpi_sys.h`
- `hi_mpi_vb.h`
- `hi_mpi_vi.h`
- `hi_mpi_vpss.h`
- `hi_mpi_venc.h`
- `hi_mpi_vda.h`
- `hi_mpi_isp.h`
- `hi_mpi_ae.h`
- `hi_mpi_awb.h`
- `mpi_mipi_rx.h`

Add `-DHISI_SDK_AVAILABLE` to `CFLAGS` in `build/sdk_path.mk` after placing the headers.
