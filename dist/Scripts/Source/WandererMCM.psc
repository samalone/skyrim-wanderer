Scriptname WandererMCM extends MCM_ConfigBase

Function ReloadSettings() native global
Function ClearOverrides() native global

Event OnSettingChange(string a_ID)
    ReloadSettings()
EndEvent
