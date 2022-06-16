Push-Location $PSScriptRoot
if (Test-Path -Path "obs-studio") {
	Push-Location obs-studio
	git pull
} else {
	git clone --recursive https://github.com/obsproject/obs-studio.git
	Push-Location obs-studio
}
./CI/build-windows.ps1
Pop-Location
Pop-Location