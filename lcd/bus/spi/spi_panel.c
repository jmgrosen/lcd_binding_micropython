#include "spi_panel.h"
#include "lcd_panel.h"

#include "modmachine.h"
#include "extmod/modmachine.h"
#include "esp32.h"
#include "softspi.h"

#include "mphalport.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

#include <string.h>


STATIC void mp_lcd_spi_panel_print(const mp_print_t *print,
                                   mp_obj_t          self_in,
                                   mp_print_kind_t   kind)
{
    (void) kind;
    mp_lcd_spi_panel_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(
        print,
        "<SPI Panel SPI=%p, dc=%p, cs=%p, width=%u, height=%u, cmd_bits=%u, param_bits=%u>",
        self->spi_obj,
        self->dc,
        self->cs,
        self->width,
        self->height,
        self->cmd_bits,
        self->param_bits
    );
}


STATIC mp_obj_t mp_lcd_spi_panel_make_new(const mp_obj_type_t *type,
                                          size_t               n_args,
                                          size_t               n_kw,
                                          const mp_obj_t      *all_args)
{
    enum {
        ARG_spi,
        ARG_dc,
        ARG_cs,
        ARG_pclk,
        ARG_width,
        ARG_height,
        ARG_cmd_bits,
        ARG_param_bits
    };
    const mp_arg_t make_new_args[] = {
        { MP_QSTR_spi,              MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED        },
        { MP_QSTR_dc,               MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED        },
        { MP_QSTR_cs,               MP_ARG_OBJ | MP_ARG_KW_ONLY,  {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_pclk,             MP_ARG_INT | MP_ARG_KW_ONLY,  {.u_int = 10000000   } },
        { MP_QSTR_width,            MP_ARG_INT | MP_ARG_KW_ONLY,  {.u_int = 240        } },
        { MP_QSTR_height,           MP_ARG_INT | MP_ARG_KW_ONLY,  {.u_int = 240        } },
        { MP_QSTR_cmd_bits,         MP_ARG_INT | MP_ARG_KW_ONLY,  {.u_int = 8          } },
        { MP_QSTR_param_bits,       MP_ARG_INT | MP_ARG_KW_ONLY,  {.u_int = 8          } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(
        n_args,
        n_kw,
        all_args,
        MP_ARRAY_SIZE(make_new_args),
        make_new_args, args
    );

    // create new object
    mp_lcd_spi_panel_obj_t *self = m_new_obj(mp_lcd_spi_panel_obj_t);
    self->base.type = &mp_lcd_spi_panel_type;

    self->spi_obj    = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[ARG_spi].u_obj);
    self->dc         = args[ARG_dc].u_obj;
    self->cs         = args[ARG_cs].u_obj;
    self->pclk       = args[ARG_pclk].u_int;
    self->width      = args[ARG_width].u_int;
    self->height     = args[ARG_height].u_int;
    self->cmd_bits   = args[ARG_cmd_bits].u_int;
    self->param_bits = args[ARG_param_bits].u_int;

#if (MICROPY_VERSION_MAJOR == 1) && (MICROPY_VERSION_MINOR >= 20)
    if (mp_obj_is_type(self->spi_obj, &machine_spi_type)) {
#else
    if (mp_obj_is_type(self->spi_obj, &machine_hw_spi_type)) {
#endif
        hal_lcd_spi_panel_construct(&self->base);
    } else if (mp_obj_is_type(self->spi_obj, &mp_machine_soft_spi_type)) {
        hal_lcd_softspi_panel_construct(&self->base);
    }

    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t mp_lcd_spi_panel_tx_param(size_t n_args, const mp_obj_t *args_in)
{
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args_in[0]);
    int cmd = mp_obj_get_int(args_in[1]);
    if (n_args == 3) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args_in[2], &bufinfo, MP_BUFFER_READ);
        hal_lcd_spi_panel_tx_param(self, cmd, bufinfo.buf, bufinfo.len);
    } else {
        hal_lcd_spi_panel_tx_param(self, cmd, NULL, 0);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_lcd_spi_panel_tx_param_obj, 2, 3, mp_lcd_spi_panel_tx_param);


STATIC mp_obj_t mp_lcd_spi_panel_tx_color(size_t n_args, const mp_obj_t *args_in)
{
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args_in[0]);
    int cmd = mp_obj_get_int(args_in[1]);

    if (n_args == 3) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args_in[2], &bufinfo, MP_BUFFER_READ);
        hal_lcd_spi_panel_tx_color(self, cmd, bufinfo.buf, bufinfo.len);
    } else {
        hal_lcd_spi_panel_tx_color(self, cmd, NULL, 0);
    }

    gc_collect();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_lcd_spi_panel_tx_color_obj, 2, 3, mp_lcd_spi_panel_tx_color);


STATIC mp_obj_t mp_lcd_spi_panel_deinit(mp_obj_t self_in)
{
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(self_in);

    hal_lcd_spi_panel_deinit(self);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_lcd_spi_panel_deinit_obj, mp_lcd_spi_panel_deinit);


STATIC const mp_rom_map_elem_t mp_lcd_spi_panel_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_tx_param), MP_ROM_PTR(&mp_lcd_spi_panel_tx_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_tx_color), MP_ROM_PTR(&mp_lcd_spi_panel_tx_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),   MP_ROM_PTR(&mp_lcd_spi_panel_deinit_obj)   },
    { MP_ROM_QSTR(MP_QSTR___del__),  MP_ROM_PTR(&mp_lcd_spi_panel_deinit_obj)   },
};
STATIC MP_DEFINE_CONST_DICT(mp_lcd_spi_panel_locals_dict, mp_lcd_spi_panel_locals_dict_table);


STATIC inline void mp_lcd_spi_panel_p_tx_param(mp_obj_base_t *self,
                                               int            lcd_cmd,
                                               const void    *param,
                                               size_t         param_size)
{
    mp_lcd_spi_panel_obj_t *spi_panel_obj = (mp_lcd_spi_panel_obj_t *)self;
#if (MICROPY_VERSION_MAJOR == 1) && (MICROPY_VERSION_MINOR >= 20)
    if (mp_obj_is_type(spi_panel_obj->spi_obj, &machine_spi_type)) {
#else
    if (mp_obj_is_type(spi_panel_obj->spi_obj, &machine_hw_spi_type)) {
#endif
        hal_lcd_spi_panel_tx_param(self, lcd_cmd, param, param_size);
    } else if (mp_obj_is_type(spi_panel_obj->spi_obj, &mp_machine_soft_spi_type)) {
        hal_lcd_softspi_panel_tx_param(self, lcd_cmd, param, param_size);
    }
}


STATIC inline void mp_lcd_spi_panel_p_tx_color(mp_obj_base_t *self,
                                               int            lcd_cmd,
                                               const void    *color,
                                               size_t         color_size)
{
    mp_lcd_spi_panel_obj_t *spi_panel_obj = (mp_lcd_spi_panel_obj_t *)self;
#if (MICROPY_VERSION_MAJOR == 1) && (MICROPY_VERSION_MINOR >= 20)
    if (mp_obj_is_type(spi_panel_obj->spi_obj, &machine_spi_type)) {
#else
    if (mp_obj_is_type(spi_panel_obj->spi_obj, &machine_hw_spi_type)) {
#endif
        hal_lcd_spi_panel_tx_color(self, lcd_cmd, color, color_size);
    } else if (mp_obj_is_type(spi_panel_obj->spi_obj, &mp_machine_soft_spi_type)) {
        hal_lcd_softspi_panel_tx_color(self, lcd_cmd, color, color_size);
    }
}


STATIC inline void mp_lcd_spi_panel_p_deinit(mp_obj_base_t *self)
{
    mp_lcd_spi_panel_obj_t *spi_panel_obj = (mp_lcd_spi_panel_obj_t *)self;
#if (MICROPY_VERSION_MAJOR == 1) && (MICROPY_VERSION_MINOR >= 20)
    if (mp_obj_is_type(spi_panel_obj->spi_obj, &machine_spi_type)) {
#else
    if (mp_obj_is_type(spi_panel_obj->spi_obj, &machine_hw_spi_type)) {
#endif
        hal_lcd_spi_panel_deinit(self);
    } else if (mp_obj_is_type(spi_panel_obj->spi_obj, &mp_machine_soft_spi_type)) {
        hal_lcd_softspi_panel_deinit(self);
    }
}


STATIC const mp_lcd_panel_p_t mp_lcd_panel_p = {
    .tx_param = mp_lcd_spi_panel_p_tx_param,
    .tx_color = mp_lcd_spi_panel_p_tx_color,
    .deinit = mp_lcd_spi_panel_p_deinit
};

#ifdef MP_OBJ_TYPE_GET_SLOT
MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_panel_type,
    MP_QSTR_SPI_Panel,
    MP_TYPE_FLAG_NONE,
    print, mp_lcd_spi_panel_print,
    make_new, mp_lcd_spi_panel_make_new,
    protocol, &mp_lcd_panel_p,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_spi_panel_locals_dict
);
#else
const mp_obj_type_t mp_lcd_spi_panel_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI_Panel,
    .print = mp_lcd_spi_panel_print,
    .make_new = mp_lcd_spi_panel_make_new,
    .protocol = &mp_lcd_panel_p,
    .locals_dict = (mp_obj_dict_t *)&mp_lcd_spi_panel_locals_dict,
};
#endif
