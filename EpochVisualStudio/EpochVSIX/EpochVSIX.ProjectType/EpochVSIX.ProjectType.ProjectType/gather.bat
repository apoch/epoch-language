@echo off
copy /y %1%2\EpochVSIX.vsix C:\Code\epoch-language\EpochVisualStudio\EpochVSIX\EpochSetupWiX\Assets\EpochVSIX.vsix
copy /y %1\BuildSystem\Rules\*.xaml C:\Code\epoch-language\EpochVisualStudio\EpochVSIX\EpochSetupWiX\Assets\Build\Rules
copy /y %1\BuildSystem\DeployedBuildSystem\*.props C:\Code\epoch-language\EpochVisualStudio\EpochVSIX\EpochSetupWiX\Assets\Build
copy /y %1\BuildSystem\DeployedBuildSystem\*.targets C:\Code\epoch-language\EpochVisualStudio\EpochVSIX\EpochSetupWiX\Assets\Build
