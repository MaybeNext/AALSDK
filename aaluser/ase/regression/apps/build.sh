# set -v

# gcc -g -o alloc_test \
#     alloc_test.c \
#     ../../sw/tstamp_ops.c \
#     ../../sw/ase_ops.c \
#     ../../sw/app_backend.c \
#     ../../sw/mqueue_ops.c \
#     ../../sw/error_report.c \
#     -lrt -lm -I ../../sw/ \
#     -D ASE_DEBUG

gcc -g -o nlb_test \
    nlb_lpbk1_test.c \
    ../../sw/tstamp_ops.c \
    ../../sw/ase_ops.c \
    ../../sw/app_backend.c \
    ../../sw/mqueue_ops.c \
    ../../sw/error_report.c \
    -lrt -lm -I ../../sw/ \
    -D ASE_DEBUG
