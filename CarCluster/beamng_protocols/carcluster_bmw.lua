-- SPDX-FileCopyrightText: 2026 WesVoj
-- SPDX-License-Identifier: GPL-3.0-only
--
-- Custom BeamNG protocol for CarCluster BMW F-series clusters.
-- Install the packaged ZIP in the BeamNG user folder under mods/.
-- For development, use mods/unpacked/carcluster_bmw_protocol/lua/vehicle/protocols/carcluster_bmw.lua.

local M = {}

local lastFuelVolume = nil
local fuelUsedAccumulator = 0
local fuelTimeAccumulator = 0
local smoothedFuelRate = 0
local getController

local function value(name, fallback)
  local v = electrics.values[name]
  if v == nil then
    return fallback
  end
  return v
end

local function numberValue(name, fallback)
  local v = value(name, fallback)
  if type(v) == "number" then
    return v
  end
  if type(v) == "boolean" then
    return v and 1 or 0
  end
  return fallback
end

local function toBool(v)
  if type(v) == "boolean" then
    return v
  end
  if type(v) == "number" then
    return v ~= 0
  end
  return v ~= nil
end

local function boolInt(name)
  return toBool(value(name, 0)) and 1 or 0
end

local function safeDamageState(group, names)
  if not damageTracker or not damageTracker.getDamage then
    return 0
  end
  for _, name in ipairs(names) do
    local ok, damaged = pcall(damageTracker.getDamage, group, name)
    if ok and toBool(damaged) then
      return 1
    end
  end
  return 0
end

local function engineDamageWarning()
  if boolInt("checkengine") ~= 0 or boolInt("checkEngine") ~= 0 then
    return 1
  end

  if powertrain and powertrain.getDevice then
    local ok, engine = pcall(powertrain.getDevice, "mainEngine")
    if ok and engine then
      if toBool(engine.isBroken) or toBool(engine.isDisabled) then
        return 1
      end
    end
  end

  return safeDamageState("engine", {
    "engineReducedTorque",
    "engineDisabled",
    "engineLockedUp",
    "engineIsHydrolocking",
    "engineHydrolocked",
    "overRevDanger",
    "catastrophicOverrevDamage",
    "mildOverrevDamage",
    "overTorqueDanger",
    "catastrophicOverTorqueDamage",
    "mildOverTorqueDamage",
    "impactDamage",
    "coolantOverheating",
    "oilOverheating",
    "oilLevelCritical",
    "oilLevelTooHigh",
    "starvedOfOil",
    "radiatorLeak",
    "oilpanLeak",
    "oilRadiatorLeak",
    "headGasketDamaged",
    "pistonRingsDamaged",
    "rodBearingsDamaged",
    "blockMelted",
    "cylinderWallsMelted",
    "inductionSystemDamaged",
    "turbochargerHot",
    "turbochargerDamaged",
    "superchargerDamaged",
    "exhaustBroken"
  })
end

local function oilLevelWarning()
  if boolInt("oil") ~= 0 or boolInt("oilWarning") ~= 0 or boolInt("oilLevelWarning") ~= 0 then
    return 1
  end

  return safeDamageState("engine", {
    "oilLevelCritical",
    "starvedOfOil",
    "oilpanLeak",
    "oilRadiatorLeak"
  })
end

local function engineOverheatWarning()
  if numberValue("watertemp", 0) >= 120 or numberValue("oiltemp", 0) >= 145 then
    return 1
  end

  return safeDamageState("engine", {
    "coolantOverheating",
    "oilOverheating",
    "headGasketDamaged",
    "blockMelted",
    "cylinderWallsMelted"
  })
end

local function signalRequest(inputName, pulseName)
  local input = electrics.values[inputName]
  if input ~= nil then
    return toBool(input) and 1 or 0
  end
  return boolInt(pulseName)
end

local function firstElectricState(names)
  for _, name in ipairs(names) do
    local v = electrics.values[name]
    if v ~= nil then
      return toBool(v) and 1 or 0, true
    end
  end
  return 0, false
end

local function firstCouplerDoorState(names)
  for _, name in ipairs(names) do
    local coupler = getController(name)
    if coupler and coupler.getGroupState then
      local ok, state = pcall(coupler.getGroupState)
      if ok and state then
        return state ~= "attached" and 1 or 0, true
      end
    end
  end
  return 0, false
end

local function doorState(couplerNames, electricNames)
  local state, known = firstCouplerDoorState(couplerNames)
  if known then
    return state, true
  end
  return firstElectricState(electricNames)
end

local function doorStates()
  local fl, flKnown = doorState(
    { "door_FL_coupler", "door_L_coupler", "doorfl_coupler" },
    { "door_FL", "doorFL", "door_fl", "doorOpenFL", "dooropenFL", "door_open_FL", "doorfrontleft", "doorFrontLeft", "door_front_left" }
  )
  local fr, frKnown = doorState(
    { "door_FR_coupler", "door_R_coupler", "doorfr_coupler" },
    { "door_FR", "doorFR", "door_fr", "doorOpenFR", "dooropenFR", "door_open_FR", "doorfrontright", "doorFrontRight", "door_front_right" }
  )
  local rl, rlKnown = doorState(
    { "door_RL_coupler", "doorrl_coupler", "barndoor_RL_coupler" },
    { "door_RL", "doorRL", "door_rl", "doorOpenRL", "dooropenRL", "door_open_RL", "doorrearleft", "doorRearLeft", "door_rear_left" }
  )
  local rr, rrKnown = doorState(
    { "door_RR_coupler", "doorrr_coupler", "barndoor_RR_coupler" },
    { "door_RR", "doorRR", "door_rr", "doorOpenRR", "dooropenRR", "door_open_RR", "doorrearright", "doorRearRight", "door_rear_right" }
  )
  local bonnet, bonnetKnown = doorState(
    { "hoodLatchCoupler", "hoodCatchCoupler", "hoodCoupler", "hood_coupler", "bonnet_coupler", "hoodLatch_coupler", "hood_latch_coupler", "hood_latch", "hood_catch" },
    { "hoodLatchCoupler_notAttached", "hoodCatchCoupler_notAttached", "hoodCoupler_notAttached", "hood_coupler_notAttached", "hood", "hoodOpen", "hood_open", "bonnet", "bonnetOpen", "bonnet_open" }
  )
  local boot, bootKnown = doorState(
    { "trunkCoupler", "tailgateCoupler", "hatchCoupler", "decklidCoupler", "bootCoupler", "liftgateCoupler", "trunk_coupler", "boot_coupler", "tailgate_coupler", "hatch_coupler", "liftgate_coupler" },
    { "trunkCoupler_notAttached", "tailgateCoupler_notAttached", "hatchCoupler_notAttached", "decklidCoupler_notAttached", "bootCoupler_notAttached", "liftgateCoupler_notAttached", "trunk", "trunkOpen", "trunk_open", "boot", "bootOpen", "boot_open", "tailgate", "tailgateOpen", "tailgate_open", "hatch", "hatchOpen", "hatch_open", "liftgate", "liftgateOpen", "liftgate_open" }
  )

  local detailsKnown = flKnown or frKnown or rlKnown or rrKnown or bonnetKnown or bootKnown
  local anyOpen = (fl ~= 0 or fr ~= 0 or rl ~= 0 or rr ~= 0 or bonnet ~= 0 or boot ~= 0)
  if not detailsKnown then
    local genericOpen = firstElectricState({ "dooropen", "doorOpen", "door_open", "doorsOpen", "doors_open" })
    if genericOpen ~= 0 then
      fl = 1
      anyOpen = true
    end
  end

  return fl, fr, rl, rr, bonnet, boot, detailsKnown and 1 or 0, anyOpen and 1 or 0
end

local function findPlain(haystack, needle)
  return string.find(haystack, needle, 1, true) ~= nil
end

local function wheelPositionFromName(name)
  local lower = string.lower(tostring(name or ""))
  local normalized = string.gsub(lower, "[^%w]+", "")
  local tokenized = "_" .. string.gsub(lower, "[^%w]+", "_") .. "_"

  if findPlain(tokenized, "_fl_") or findPlain(tokenized, "_lf_") or
     findPlain(normalized, "frontleft") or findPlain(normalized, "leftfront") then
    return "fl"
  end
  if findPlain(tokenized, "_fr_") or findPlain(tokenized, "_rf_") or
     findPlain(normalized, "frontright") or findPlain(normalized, "rightfront") then
    return "fr"
  end
  if findPlain(tokenized, "_rl_") or findPlain(tokenized, "_lr_") or
     findPlain(normalized, "rearleft") or findPlain(normalized, "leftrear") or
     findPlain(normalized, "backleft") or findPlain(normalized, "leftback") then
    return "rl"
  end
  if findPlain(tokenized, "_rr_") or findPlain(tokenized, "_rright_") or
     findPlain(normalized, "rearright") or findPlain(normalized, "rightrear") or
     findPlain(normalized, "backright") or findPlain(normalized, "rightback") then
    return "rr"
  end

  local hasFront = findPlain(normalized, "front")
  local hasRear = findPlain(normalized, "rear") or findPlain(normalized, "back")
  local hasLeft = findPlain(normalized, "left")
  local hasRight = findPlain(normalized, "right")
  if hasFront and hasLeft then return "fl" end
  if hasFront and hasRight then return "fr" end
  if hasRear and hasLeft then return "rl" end
  if hasRear and hasRight then return "rr" end

  return nil
end

local function wheelPositionFromId(wheelID)
  if wheelID == 0 then return "fl" end
  if wheelID == 1 then return "fr" end
  if wheelID == 2 then return "rl" end
  if wheelID == 3 then return "rr" end
  return nil
end

local function wheelPressure(wd)
  if not wd then
    return nil
  end

  local pressureGroupId = wd.pressureGroupId
  if not pressureGroupId and wd.pressureGroup and v and v.data and v.data.pressureGroups then
    pressureGroupId = v.data.pressureGroups[wd.pressureGroup]
  end
  if not pressureGroupId or not obj or not obj.getGroupPressure then
    return nil
  end

  local ok, pressure = pcall(function() return obj:getGroupPressure(pressureGroupId) end)
  if ok and type(pressure) == "number" then
    return pressure
  end
  return nil
end

local function environmentPressure()
  if obj and obj.getEnvPressure then
    local ok, pressure = pcall(function() return obj:getEnvPressure() end)
    if ok and type(pressure) == "number" then
      return pressure
    end
  end
  return 101325
end

local function isWheelDeflated(wd)
  if not wd then
    return false
  end
  if wd.isPunctured or wd.isTireDeflated then
    return true
  end

  local pressure = wheelPressure(wd)
  if not pressure then
    return false
  end

  local envPressure = environmentPressure()
  if pressure <= envPressure + 30000 then
    return true
  end

  local startingPressure = tonumber(wd.startingPressure)
  return startingPressure ~= nil and startingPressure > 0 and pressure < startingPressure * 0.55
end

local function tireWarningStates()
  local fl, fr, rl, rr = 0, 0, 0, 0
  if not wheels or not wheels.wheels then
    return fl, fr, rl, rr
  end

  for _, wd in pairs(wheels.wheels) do
    if isWheelDeflated(wd) then
      local position = wheelPositionFromName(wd.name) or wheelPositionFromId(wd.wheelID)
      if position == "fl" then
        fl = 1
      elseif position == "fr" then
        fr = 1
      elseif position == "rl" then
        rl = 1
      elseif position == "rr" then
        rr = 1
      end
    end
  end

  return fl, fr, rl, rr
end

function getController(name)
  if not controller or not controller.getController then
    return nil
  end

  local ok, result = pcall(controller.getController, name)
  if ok then
    return result
  end
  return nil
end

local function driveModeText()
  local parts = {}
  local driveModes = getController("driveModes")
  if driveModes then
    local key = driveModes.getCurrentDriveModeKey and driveModes.getCurrentDriveModeKey()
    if key then
      table.insert(parts, tostring(key))
    end

    if key and driveModes.getDriveModeData then
      local mode = driveModes.getDriveModeData(key)
      if mode and mode.name then
        table.insert(parts, tostring(mode.name))
      end
    end
  end

  local esc = getController("esc")
  if esc and esc.getCurrentConfigData then
    local config = esc.getCurrentConfigData()
    if config and config.name then
      table.insert(parts, tostring(config.name))
    end
  end

  return table.concat(parts, " ")
end

local function mapDriveMode()
  local text = string.lower(driveModeText())
  local normalized = string.gsub(text, "%+", "plus")
  normalized = string.gsub(normalized, "&", "and")
  normalized = string.gsub(normalized, "[^%w]+", "")

  if findPlain(normalized, "traction") or findPlain(normalized, "offroad") then
    return 1
  end
  if findPlain(normalized, "drift") or findPlain(normalized, "escandtcoff") or findPlain(normalized, "escandtractioncontroloff") or findPlain(normalized, "escoff") or findPlain(normalized, "tcoff") then
    return 6
  end
  if findPlain(normalized, "sportplus") or findPlain(normalized, "ttsportplus") then
    return 5
  end
  if findPlain(normalized, "sport") or findPlain(normalized, "ttsport") then
    return 4
  end
  if findPlain(normalized, "ecopro") or findPlain(normalized, "eco") then
    return 7
  end
  if findPlain(normalized, "comfort") then
    local gearText = tostring(value("gear", ""))
    if string.upper(string.sub(gearText, 1, 1)) == "D" then
      return 7
    end
    return 2
  end

  return 2
end

local function mapGear()
  local gearText = string.upper(tostring(value("gear", "")))
  local gearPrefix = string.sub(gearText, 1, 1)

  if gearPrefix == "P" then
    return 255
  elseif gearPrefix == "R" then
    return 0
  elseif gearPrefix == "N" then
    return 1
  elseif gearPrefix == "S" then
    return 254
  elseif gearPrefix == "D" then
    return 10
  elseif gearPrefix == "M" then
    local gearIndex = numberValue("gearIndex", 0)
    if gearIndex > 0 then
      return gearIndex + 1
    end
    return 254
  end

  return numberValue("gearIndex", 0) + 1
end

local function init() end

local function reset()
  lastFuelVolume = nil
  fuelUsedAccumulator = 0
  fuelTimeAccumulator = 0
  smoothedFuelRate = 0
end

local function getAddress()
  local configured = settings.getValue("protocols_outgauge_address")
  if configured and configured ~= "" then
    return configured
  end
  return "192.168.4.1"
end

local function getPort()
  local configured = tonumber(settings.getValue("protocols_outgauge_port"))
  if configured and configured > 0 then
    return configured
  end
  return 1102
end

local function getMaxUpdateRate()
  return 60
end

local function isPhysicsStepUsed()
  return false
end

local function getStructDefinition()
  return [[
    char     magic[4];
    unsigned version;
    unsigned timeMs;
    float    speedKmh;
    float    rpm;
    float    coolantTempC;
    float    oilTempC;
    float    fuelRatio;
    float    fuelVolumeLiters;
    float    fuelUsedLiters;
    float    fuelRateLitersPerHour;
    float    throttle;
    float    brake;
    float    clutch;
    float    engineLoad;
    unsigned gear;
    unsigned ignitionLevel;
    unsigned lowBeam;
    unsigned highBeam;
    unsigned leftSignal;
    unsigned rightSignal;
    unsigned parkingBrake;
    unsigned absActive;
    unsigned escActive;
    unsigned batteryWarning;
    unsigned oilWarning;
    unsigned driveMode;
    float    outdoorTempC;
    unsigned doorFrontLeft;
    unsigned doorFrontRight;
    unsigned doorRearLeft;
    unsigned doorRearRight;
    unsigned doorDetailsAvailable;
    unsigned doorAnyOpen;
    unsigned tireWarningFrontLeft;
    unsigned tireWarningFrontRight;
    unsigned tireWarningRearLeft;
    unsigned tireWarningRearRight;
    unsigned doorBonnet;
    unsigned doorBoot;
    unsigned engineDamageWarning;
    unsigned cruiseControlActive;
    float    cruiseControlSetSpeedKmh;
    unsigned oilLevelWarning;
    unsigned engineOverheatWarning;
  ]]
end

local function fillStruct(o, dtSim)
  if not electrics.values.watertemp then
    return
  end

  local fuelVolume = numberValue("fuelVolume", 0)
  local fuelCapacity = numberValue("fuelCapacity", 0)
  local fuelRatio = numberValue("fuel", 0)
  if fuelCapacity > 0 then
    fuelRatio = fuelVolume / fuelCapacity
  end
  if fuelRatio < 0 then
    fuelRatio = 0
  elseif fuelRatio > 1 then
    fuelRatio = 1
  end

  local fuelUsed = 0
  if lastFuelVolume ~= nil and lastFuelVolume > fuelVolume then
    fuelUsed = lastFuelVolume - fuelVolume
  end
  lastFuelVolume = fuelVolume

  local sampleTime = dtSim or 0
  if sampleTime > 0 then
    fuelUsedAccumulator = fuelUsedAccumulator + fuelUsed
    fuelTimeAccumulator = fuelTimeAccumulator + sampleTime
    if fuelTimeAccumulator >= 0.75 then
      local rawFuelRate = 0
      if fuelUsedAccumulator > 0 then
        rawFuelRate = fuelUsedAccumulator * 3600 / fuelTimeAccumulator
      end
      smoothedFuelRate = smoothedFuelRate * 0.65 + rawFuelRate * 0.35
      fuelUsedAccumulator = 0
      fuelTimeAccumulator = 0
    end
  end

  o.magic = "CCB1"
  o.version = 1
  o.timeMs = 0
  local doorFL, doorFR, doorRL, doorRR, doorBonnet, doorBoot, doorDetailsAvailable, doorAnyOpen = doorStates()
  local tireFL, tireFR, tireRL, tireRR = tireWarningStates()
  o.speedKmh = numberValue("wheelspeed", numberValue("airspeed", 0)) * 3.6
  o.rpm = numberValue("rpmTacho", numberValue("rpm", 0))
  o.coolantTempC = numberValue("watertemp", 0)
  o.oilTempC = numberValue("oiltemp", 0)
  o.fuelRatio = fuelRatio
  o.fuelVolumeLiters = fuelVolume
  o.fuelUsedLiters = fuelUsed
  o.fuelRateLitersPerHour = smoothedFuelRate
  o.throttle = numberValue("throttle", numberValue("throttle_input", 0))
  o.brake = numberValue("brake", numberValue("brake_input", 0))
  o.clutch = numberValue("clutch", numberValue("clutch_input", 0))
  o.engineLoad = numberValue("engineLoad", 0)
  o.gear = mapGear()
  o.ignitionLevel = numberValue("ignitionLevel", boolInt("engineRunning"))
  o.lowBeam = boolInt("lowbeam")
  o.highBeam = boolInt("highbeam")
  o.leftSignal = (boolInt("hazard_enabled") ~= 0 or signalRequest("signal_left_input", "signal_L") ~= 0) and 1 or 0
  o.rightSignal = (boolInt("hazard_enabled") ~= 0 or signalRequest("signal_right_input", "signal_R") ~= 0) and 1 or 0
  o.parkingBrake = boolInt("parkingbrake")
  o.absActive = boolInt("absActive")
  o.escActive = (boolInt("esc") ~= 0 or boolInt("tcs") ~= 0) and 1 or 0
  o.batteryWarning = boolInt("engineRunning") == 0 and 1 or 0
  o.oilWarning = boolInt("oil")
  o.driveMode = mapDriveMode()
  o.outdoorTempC = numberValue("ambientTemperature", numberValue("airTemperature", numberValue("envTemperature", 20)))
  o.doorFrontLeft = doorFL
  o.doorFrontRight = doorFR
  o.doorRearLeft = doorRL
  o.doorRearRight = doorRR
  o.doorDetailsAvailable = doorDetailsAvailable
  o.doorAnyOpen = doorAnyOpen
  o.tireWarningFrontLeft = tireFL
  o.tireWarningFrontRight = tireFR
  o.tireWarningRearLeft = tireRL
  o.tireWarningRearRight = tireRR
  o.doorBonnet = doorBonnet
  o.doorBoot = doorBoot
  o.engineDamageWarning = engineDamageWarning()

  -- Cruise control. BeamNG exposes cruiseControlActive (bool) and
  -- cruiseControlTarget (target speed in m/s). Convert the target to km/h for the
  -- cluster, which positions the green speedo-ring marker at that speed.
  o.cruiseControlActive = boolInt("cruiseControlActive")
  local ccTargetMs = numberValue("cruiseControlTarget", 0)
  o.cruiseControlSetSpeedKmh = ccTargetMs * 3.6
  o.oilLevelWarning = oilLevelWarning()
  o.engineOverheatWarning = engineOverheatWarning()
end

M.init = init
M.reset = reset
M.getAddress = getAddress
M.getPort = getPort
M.getMaxUpdateRate = getMaxUpdateRate
M.getStructDefinition = getStructDefinition
M.fillStruct = fillStruct
M.isPhysicsStepUsed = isPhysicsStepUsed

return M
