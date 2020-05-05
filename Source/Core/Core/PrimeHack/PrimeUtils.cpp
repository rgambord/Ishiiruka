#include "Core/PrimeHack/PrimeUtils.h"

std::string info_str;

namespace prime
{
  /*
  * Prime one beam IDs: 0 = power, 1 = ice, 2 = wave, 3 = plasma
  * Prime one visor IDs: 0 = combat, 1 = xray, 2 = scan, 3 = thermal
  * Prime two beam IDs: 0 = power, 1 = dark, 2 = light, 3 = annihilator
  * Prime two visor IDs: 0 = combat, 1 = echo, 2 = scan, 3 = dark
  * ADDITIONAL INFO: Equipment have-status offsets:
  * Beams can be ignored (for now) as the existing code handles that for us
  * Prime one visor offsets: combat = 0x11, scan = 0x05, thermal = 0x09, xray = 0x0d
  * Prime two visor offsets: combat = 0x08, scan = 0x09, dark = 0x0a, echo = 0x0b
  */

  static float cursor_x = 0, cursor_y = 0;
  static int current_beam = 0;
  static int current_visor = 0;
  static std::array<bool, 4> beam_owned = {false, false, false, false};
  static std::array<bool, 4> visor_owned = {false, false, false, false};
  static bool noclip_enabled = false;

  std::array<std::array<CodeChange, static_cast<int>(Game::MAX_VAL) + 1>,
    static_cast<int>(Region::MAX_VAL) + 1> noclip_enable_codes;
  std::array<std::array<CodeChange, static_cast<int>(Game::MAX_VAL) + 1>,
    static_cast<int>(Region::MAX_VAL) + 1> noclip_disable_codes;

  void MenuNTSC::run_mod() {
    handle_cursor(0x80913c9c, 0x80913d5c, 0.95f, 0.90f);
  }

  void MenuPAL::run_mod() {
    u32 cursor_base = PowerPC::HostRead_U32(0x80621ffc);
    handle_cursor(cursor_base + 0xdc, cursor_base + 0x19c, 0.95f, 0.90f);
  }

  constexpr u8 read8(u32 addr) {
    return PowerPC::HostRead_U8(addr);
  }

  constexpr u16 read16(u32 addr) {
    return PowerPC::HostRead_U16(addr);
  }

  constexpr u32 read32(u32 addr) {
    return PowerPC::HostRead_U32(addr);
  }

  constexpr u32 readi(u32 addr) {
    return PowerPC::HostRead_Instruction(addr);
  }

  constexpr u64 read64(u32 addr) {
    return PowerPC::HostRead_U64(addr);
  }

  constexpr void write8(u8 var, u32 addr) {
    PowerPC::HostWrite_U8(var, addr);
  }

  constexpr void write16(u16 var, u32 addr) {
    PowerPC::HostWrite_U16(var, addr);
  }

  constexpr void write32(u32 var, u32 addr) {
    PowerPC::HostWrite_U32(var, addr);
  }

  constexpr void write64(u64 var, u32 addr) {
    PowerPC::HostWrite_U64(var, addr);
  }

  constexpr void writef32(float var, u32 addr) {
    PowerPC::HostWrite_F32(var, addr);
  }

  void set_beam_owned(int index, bool owned)
  {
    beam_owned[index] = owned;
  }

  void set_visor_owned(int index, bool owned)
  {
    visor_owned[index] = owned;
  }

  void set_cursor_pos(float x, float y)
  {
    cursor_x = x;
    cursor_y = y;
  }

  void disable_culling(u32 address)
  {
    write_invalidate(address, 0x38600001);
    write_invalidate(address + 0x4, 0x4E800020);
  }

  void write_invalidate(u32 address, u32 value)
  {
    PowerPC::HostWrite_U32(value, address);
    PowerPC::ScheduleInvalidateCacheThreadSafe(address);
  }

  void write_if_different(u32 address, u32 value)
  {
    if (PowerPC::HostRead_U32(address) != value)
      write_invalidate(address, value);
  }

  std::tuple<int, int> get_visor_switch(std::array<std::tuple<int, int>, 4> const& visors, bool combat_visor)
  {
    static bool pressing_button = false;
    if (CheckVisorCtl(0))
    {
      if (!combat_visor)
      {
        if (!pressing_button)
        {
          pressing_button = true;
          return visors[0];
        }
        else {
          return std::make_tuple(-1, 0);
        }
      }
    }

    if (CheckVisorCtl(1))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        return visors[1];
      }
    }
    else if (CheckVisorCtl(2))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        return visors[2];
      }
    }
    else if (CheckVisorCtl(3))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        return visors[3];
      }
    }
    else if (CheckVisorScrollCtl(true))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        for (int i = 0; i < 4; i++) {
          if (visor_owned[current_visor = (current_visor + 1) % 4]) break;
        }
        return visors[current_visor];
      }
    }
    else if (CheckVisorScrollCtl(false))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        for (int i = 0; i < 4; i++) {
          if (visor_owned[current_visor = (current_visor + 3) % 4]) break;
        }
        return visors[current_visor];
      }
    }
    else
    {
      pressing_button = false;
    }
    return std::make_tuple(-1, 0);
  }

  std::stringstream ss;
  void DevInfo(std::string line, u32 hex)
  {
    ss << line << ": " << std::hex << hex << std::endl;
  }

  std::string GetDevInfo()
  {
    std::string result = ss.str();

    return result;
  }

  void ClrDevInfo()
  {
    ss = std::stringstream();
  }

  int get_beam_switch(std::array<int, 4> const& beams)
  {
    static bool pressing_button = false;
    if (CheckBeamCtl(0))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        return current_beam = beams[0];
      }
    }
    else if (CheckBeamCtl(1))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        return current_beam = beams[1];
      }
    }
    else if (CheckBeamCtl(2))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        return current_beam = beams[2];
      }
    }
    else if (CheckBeamCtl(3))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        return current_beam = beams[3];
      }
    }
    else if (CheckBeamScrollCtl(true))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        for (int i = 0; i < 4; i++) {
          if (beam_owned[current_beam = (current_beam + 1) % 4]) break;
        }
        return beams[current_beam];
      }
    }
    else if (CheckBeamScrollCtl(false))
    {
      if (!pressing_button)
      {
        pressing_button = true;
        for (int i = 0; i < 4; i++) {
          if (beam_owned[current_beam = (current_beam + 3) % 4]) break;
        }
        return beams[current_beam];
      }
    }
    else
    {
      pressing_button = false;
    }
    return -1;
  }

  void adjust_viewmodel(float fov, u32 arm_address, u32 znear_address, u32 znear_value)
  {
    float left = 0.25f;
    float forward = 0.30f;
    float up = -0.35f;
    bool apply_znear = false;

    if (GetToggleArmAdjust())
    {
      if (GetAutoArmAdjust()) {
        if (fov > 125)
        {
          left = 0.22f;
          forward = -0.02f;

          apply_znear = true;
        }
        else if (fov >= 75)
        {
          left = -0.00020000000000000017f * fov + 0.265f;
          forward = -0.005599999999999999f * fov + 0.72f;

          apply_znear = true;
        }
      }
      else {
        std::tie<float, float, float>(left, forward, up) = GetArmXYZ();
        apply_znear = true;
      }
    }

    if (apply_znear)
      PowerPC::HostWrite_U32(znear_value, znear_address);

    PowerPC::HostWrite_F32(left, arm_address);
    PowerPC::HostWrite_F32(forward, arm_address + 0x4);
    PowerPC::HostWrite_F32(up, arm_address + 0x8);
  }

  bool mem_check(u32 address)
  {
    return (address >= 0x80000000) && (address < 0x81800000);
  }

  float getAspectRatio()
  {
    const unsigned int scale = g_renderer->GetEFBScale();
    float sW = scale * EFB_WIDTH;
    float sH = scale * EFB_HEIGHT;
    return sW / sH;
  }

  void handle_cursor(u32 x_address, u32 y_address, float right_bound, float bottom_bound)
  {
    int dx = GetHorizontalAxis(), dy = GetVerticalAxis();

    float aspect_ratio = getAspectRatio();
    if (std::isnan(aspect_ratio))
      return;
    const float cursor_sensitivity_conv = prime::GetCursorSensitivity() / 10000.f;
    cursor_x += static_cast<float>(dx) * cursor_sensitivity_conv;
    cursor_y += static_cast<float>(dy) * cursor_sensitivity_conv * aspect_ratio;

    cursor_x = std::clamp(cursor_x, -1.f, right_bound);
    cursor_y = std::clamp(cursor_y, -1.f, bottom_bound);

    PowerPC::HostWrite_U32(*reinterpret_cast<u32*>(&cursor_x), x_address);
    PowerPC::HostWrite_U32(*reinterpret_cast<u32*>(&cursor_y), y_address);
  }

  void springball_code(u32 base_offset, std::vector<CodeChange>* code_changes)
  {
    code_changes->emplace_back(base_offset + 0x00, 0x3C808000);  // lis r4, 0x8000
    code_changes->emplace_back(base_offset + 0x04, 0x60844164);  // ori r4, r4, 0x4164
    code_changes->emplace_back(base_offset + 0x08, 0x88640000);  // lbz r3, 0(r4)
    code_changes->emplace_back(base_offset + 0x0c, 0x38A00000);  // li r5, 0
    code_changes->emplace_back(base_offset + 0x10, 0x98A40000);  // stb r5, 0(r4)
    code_changes->emplace_back(base_offset + 0x14, 0x2C030000);  // cmpwi r3, 0
  }

  void springball_check(u32 ball_address, u32 movement_address)
  {
    if (CheckSpringBallCtl())
    {             
      u32 ball_state = PowerPC::HostRead_U32(ball_address);
      u32 movement_state = PowerPC::HostRead_U32(movement_address);

      if ((ball_state == 1 || ball_state == 2) && movement_state == 0)
        PowerPC::HostWrite_U8(1, 0x80004164);
    }
  }

  
  static Transform player_tf;
  static vec3 player_pos;

  void register_noclip_enable(CodeChange enable, CodeChange disable, Game game, Region region) {
    noclip_enable_codes[static_cast<int>(region)][static_cast<int>(game)] = enable;
    noclip_disable_codes[static_cast<int>(region)][static_cast<int>(game)] = disable;
  }

  void toggle_noclip(u32 player_tf_addr) {
    int game = static_cast<int>(GetHackManager()->get_active_game());
    int region = static_cast<int>(GetHackManager()->get_active_region());
    noclip_enabled = !noclip_enabled;
    if (noclip_enabled) {
      get_tf(player_tf_addr, player_tf);
      write_invalidate(noclip_enable_codes[region][game].address,
        noclip_enable_codes[region][game].var);
    }
    else {
      write_invalidate(noclip_disable_codes[region][game].address,
        noclip_disable_codes[region][game].var);
    }
  }

  void toggle_noclip_mp2(u32 player_pos_addr) {
    int game = static_cast<int>(GetHackManager()->get_active_game());
    int region = static_cast<int>(GetHackManager()->get_active_region());
    noclip_enabled = !noclip_enabled;
    if (noclip_enabled) {
      *((u32*)&player_pos.x) = PowerPC::HostRead_U32(player_pos_addr);
      *((u32*)&player_pos.y) = PowerPC::HostRead_U32(player_pos_addr + 4);
      *((u32*)&player_pos.z) = PowerPC::HostRead_U32(player_pos_addr + 8);
      write_invalidate(noclip_enable_codes[region][game].address,
        noclip_enable_codes[region][game].var);
    }
    else {
      write_invalidate(noclip_disable_codes[region][game].address,
        noclip_disable_codes[region][game].var);
    }
  }

  void noclip_mp2(u32 player_pos_addr, u32 camera_tf_addr, bool has_control) {
    int game = static_cast<int>(GetHackManager()->get_active_game());
    int region = static_cast<int>(GetHackManager()->get_active_region());
    static bool was_controlling = true;
    if (!has_control) {
      was_controlling = has_control;
      *((u32*)&player_pos.x) = PowerPC::HostRead_U32(player_pos_addr);
      *((u32*)&player_pos.y) = PowerPC::HostRead_U32(player_pos_addr + 4);
      *((u32*)&player_pos.z) = PowerPC::HostRead_U32(player_pos_addr + 8);
      write_invalidate(noclip_disable_codes[region][game].address,
        noclip_disable_codes[region][game].var);
      return;
    }
    if (has_control && !was_controlling && noclip_enabled) {
      write_invalidate(noclip_enable_codes[region][game].address,
        noclip_enable_codes[region][game].var);
    }


    if (noclip_enabled) {
      Transform camera_tf;
      get_tf(camera_tf_addr, camera_tf);

      vec3 movement_vec;
      if (CheckForward()) {
        movement_vec = movement_vec + camera_tf.fwd();
      }
      if (CheckBack()) {
        movement_vec = movement_vec + -camera_tf.fwd();
      }
      if (CheckLeft()) {
        movement_vec = movement_vec + -camera_tf.right();
      }
      if (CheckRight()) {
        movement_vec = movement_vec + camera_tf.right();
      }

      movement_vec = (movement_vec * 0.5f) + player_pos;
      player_pos = movement_vec;
      PowerPC::HostWrite_F32(movement_vec.x, player_pos_addr);
      PowerPC::HostWrite_F32(movement_vec.y, player_pos_addr + 0x04);
      PowerPC::HostWrite_F32(movement_vec.z, player_pos_addr + 0x08);
    }
  }

  void noclip(u32 player_tf_addr, u32 camera_tf_addr, bool has_control) {
    int game = static_cast<int>(GetHackManager()->get_active_game());
    int region = static_cast<int>(GetHackManager()->get_active_region());
    static bool was_controlling = false;
    if (!has_control) {
      write_invalidate(noclip_disable_codes[region][game].address,
        noclip_disable_codes[region][game].var);
      was_controlling = has_control;
      get_tf(player_tf_addr, player_tf);
      return;
    }
    if (has_control && !was_controlling && noclip_enabled) {
      write_invalidate(noclip_enable_codes[region][game].address,
        noclip_enable_codes[region][game].var);
    }

    was_controlling = has_control;
    
    if (noclip_enabled) {
      Transform camera_tf;
      get_tf(camera_tf_addr, camera_tf);

      vec3 movement_vec;
      if (CheckForward()) {
        movement_vec = movement_vec + camera_tf.fwd();
      }
      if (CheckBack()) {
        movement_vec = movement_vec + -camera_tf.fwd();
      }
      if (CheckLeft()) {
        movement_vec = movement_vec + -camera_tf.right();
      }
      if (CheckRight()) {
        movement_vec = movement_vec + camera_tf.right();
      }

      movement_vec = (movement_vec * 0.5f) + player_tf.loc();
      player_tf.update_loc(movement_vec);
      PowerPC::HostWrite_F32(movement_vec.x, player_tf_addr + 0x0C);
      PowerPC::HostWrite_F32(movement_vec.y, player_tf_addr + 0x1C);
      PowerPC::HostWrite_F32(movement_vec.z, player_tf_addr + 0x2C);
    }
  }
}  // namespace prime
