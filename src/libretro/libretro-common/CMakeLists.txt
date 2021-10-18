add_definitions("-DHAVE_RBMP")
add_definitions("-DHAVE_RPNG")
add_definitions("-DHAVE_ZLIB")
add_definitions("-DHAVE_NEAREST_RESAMPLER")

add_library(libretro-common
    ../libretro-uncommon/formats/bmp/rbmp.c
    ../libretro-uncommon/formats/image_texture.c

    formats/png/rpng.c
    formats/image_transfer.c
    compat/compat_strl.c
    string/stdstring.c
    streams/file_stream.c
    streams/trans_stream.c
    streams/trans_stream_pipe.c
    streams/trans_stream_zlib.c
    file/nbio/nbio_intf.c
    file/nbio/nbio_linux.c
    file/nbio/nbio_stdio.c
    file/file_path.c

    memmap/memalign.c

    audio/audio_mix.c
    audio/conversion/float_to_s16.c
    audio/conversion/s16_to_float.c
    audio/resampler/audio_resampler.c
    audio/resampler/drivers/nearest_resampler.c
    audio/resampler/drivers/sinc_resampler.c

    vfs/vfs_implementation.c

    features/features_cpu.c
    formats/wav/rwav.c
    file/config_file_userdata.c
)
