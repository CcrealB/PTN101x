# SDK common include path

set(INC_PATH
"${CMAKE_SOURCE_DIR}/host/include" 
"${CMAKE_SOURCE_DIR}/host/port/beken_ctrl" 
"${CMAKE_SOURCE_DIR}/host/sys/include" 
"${CMAKE_SOURCE_DIR}/host/bluetooth/protocol/avctp" 
"${CMAKE_SOURCE_DIR}/host/bluetooth/protocol/avdtp" 
"${CMAKE_SOURCE_DIR}/controller/core/le/include" 
"${CMAKE_SOURCE_DIR}/host/port/include/os" 
"${CMAKE_SOURCE_DIR}/host" 
"${CMAKE_SOURCE_DIR}/host/bluetooth/core" 
"${CMAKE_SOURCE_DIR}/host/bluetooth" 
"${CMAKE_SOURCE_DIR}/host/bluetooth/profile/a2dp" 
"${CMAKE_SOURCE_DIR}/host/bluetooth/include" 
"${CMAKE_SOURCE_DIR}/host/config" 
"${CMAKE_SOURCE_DIR}/host/jos" 
"${CMAKE_SOURCE_DIR}/host/pkg/sbc"
"${CMAKE_SOURCE_DIR}/host/jos/include" 
"${CMAKE_SOURCE_DIR}/host/port" 
"${CMAKE_SOURCE_DIR}/host/port/audio" 
"${CMAKE_SOURCE_DIR}/host/port/beken_app" 
"${CMAKE_SOURCE_DIR}/host/port/beken_driver" 
"${CMAKE_SOURCE_DIR}/host/port/beken_no_os" 
"${CMAKE_SOURCE_DIR}/host/port/common" 
"${CMAKE_SOURCE_DIR}/host/port/include" 
"${CMAKE_SOURCE_DIR}/host/port/mod_dsp"
"${CMAKE_SOURCE_DIR}/host/port/mod_spdif"
"${CMAKE_SOURCE_DIR}/host/port/mod_recoder"
"${CMAKE_SOURCE_DIR}/host/port/usb_driver"
"${CMAKE_SOURCE_DIR}/host/port/common/bluetooth" 
"${CMAKE_SOURCE_DIR}/host/port/include/bluetooth" 
"${CMAKE_SOURCE_DIR}/controller/core/bt/include" 
"${CMAKE_SOURCE_DIR}/controller/core/hc/include" 
"${CMAKE_SOURCE_DIR}/controller/core/lc/include" 
"${CMAKE_SOURCE_DIR}/controller/core/lc/dl/include" 
"${CMAKE_SOURCE_DIR}/controller/core/lc/uslc/include" 
"${CMAKE_SOURCE_DIR}/controller/core/lc/lslc/include" 
"${CMAKE_SOURCE_DIR}/controller/core/lmp/include" 
"${CMAKE_SOURCE_DIR}/controller/core/sys/include" 
"${CMAKE_SOURCE_DIR}/controller/core/tc/include" 
"${CMAKE_SOURCE_DIR}/controller/core/transport/include" 
"${CMAKE_SOURCE_DIR}/controller/core/hw/include" 
"${CMAKE_SOURCE_DIR}/controller/hal/hw/include" 
"${CMAKE_SOURCE_DIR}/controller/hal/hw/radio/include" 
"${CMAKE_SOURCE_DIR}/controller/hal/beken/hw/include" 
"${CMAKE_SOURCE_DIR}/controller/hal/beken/sys/include" 
"${CMAKE_SOURCE_DIR}/host/libs/AAC" 
"${CMAKE_SOURCE_DIR}/host/libs/sbc" 
"${CMAKE_SOURCE_DIR}/host/libs/Mp3Lib" 
"${CMAKE_SOURCE_DIR}/host/libs/FatLib" 
"${CMAKE_SOURCE_DIR}/host/libs/AEC" 
"${CMAKE_SOURCE_DIR}/host/libs/RESAMPLE" 
"${CMAKE_SOURCE_DIR}/host/libs/asrc"
"${CMAKE_SOURCE_DIR}/rw_ble/project/app_peripheral/app/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/project/app_peripheral/app/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/project/app_peripheral/config" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/driver/uart" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/header" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/inc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/ll/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/ll/import" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/ll/import/reg" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/import" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/import/reg" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/em/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/hci/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/sch/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/sch/import" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/aes/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/common/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/dbg/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/display/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/ecc_p256/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/h4tl/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/ke/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/nvds/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/rf/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/rwip/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/rwip/import" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/rwip/import/reg" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/plf/prf" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/plf/refip/import" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/plf/refip/import/reg" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hrp/hrpc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/sdp/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/system" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/plf/refip/src/arch/compiler" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/plf/refip/src/arch" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/plf/refip/src/arch/ll" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gatt/gattc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gatt/gattm" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gatt" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gatt/attc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gatt/attm" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gatt/atts" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gap/gapc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/gap/gapm" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/ke/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hrp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/ll/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/project/app_central/app/src"
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp/hogprh" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp/hogprh/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp/hogprh/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/FFF0/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/FFF0/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/FEE0/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/FEE0/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/project/app_peripheral/config" 
"${CMAKE_SOURCE_DIR}/rw_ble/project/app_peripheral/app/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/project/app_peripheral/app/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/l2c/l2cc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/hl/src/l2c/l2cm" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/ll/src/llc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/ll/src/lld" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/ble/ll/src/llm" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/src/lb" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/src/lc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/src/ld" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/bt/src/lm" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/ip/hci/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/aes/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/dbg/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/display/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/modules/rwip/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/anp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/anp/anpc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/ancs/ancsc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/ancs/ancsc" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/anp/anpc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/anp/anps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/anp/anps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bas/basc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bas/basc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bas/bass/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bas/bass/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bcs" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bcs/bcsc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bcs/bcsc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bcs/bcss/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/bcs/bcss/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/blp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/blp/blpc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/blp/blpc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/blp/blps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/blp/blps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cpp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cpp/cppc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cpp/cppc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cpp/cpps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cpp/cpps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cscp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cscp/cscpc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cscp/cscpc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cscp/cscps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/cscp/cscps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/dis/disc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/dis/disc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/dis/diss/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/dis/diss/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/electric/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/FCC0_128/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/find" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/find/findl/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/find/findl/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/find/findt/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/find/findt/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/glp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/glp/glpc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/glp/glpc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/glp/glps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp/hogpbh/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp/hogpbh/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp/hogpd/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hogp/hogpd/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hrp/hrpc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hrp/hrps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/hrp/hrps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/htp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/htp/htpc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/htp/htpc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/htp/htpt/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/htp/htpt/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/lan" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/lan/lanc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/lan/lanc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/lan/lans/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/lan/lans/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/pasp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/pasp/paspc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/pasp/paspc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/pasp/pasps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/pasp/pasps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/prox/proxm/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/prox/proxm/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/prox/proxr/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/prox/proxr/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/rscp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/rscp/rscpc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/rscp/rscpc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/rscp/rscps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/rscp/rscps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/scpp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/scpp/scppc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/scpp/scppc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/scpp/scpps/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/scpp/scpps/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/tip" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/tip/tipc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/tip/tipc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/tip/tips/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/tip/tips/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/uds" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/uds/udsc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/uds/udsc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/uds/udss/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/uds/udss/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/wscp" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/wscp/wscc/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/wscp/wscc/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/wscp/wscs/api" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/wscp/wscs/src" 
"${CMAKE_SOURCE_DIR}/rw_ble/Src/profiles/ota/api"
${INC_PATH}
)