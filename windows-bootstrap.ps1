#Requires -Version 7.5

param (
	[string]$WorkingDirectory=$PSScriptRoot,
	[string]$QtVersionString="6.11.1",
	[string]$QtUnpackedArchiveName="qt-everywhere-src-${QtVersionString}",
	[string]$QtWorkingDirectory=(Join-Path -Path $WorkingDirectory -ChildPath $QtUnpackedArchiveName),
	[string]$OpenSSLVersion="3.6.5",
	[string]$OpenSSLWorkingDirectory=(Join-Path -Path $WorkingDirectory -ChildPath "openssl-openssl-${OpenSSLVersion}"),
	[switch]$SkipOpenSSL,
	[string]$OBSVersion="32.2.2",
	[string]$OBSWorkingDirectory="obs-studio",
	[string]$OBSUnpackedArchiveName="obs-studio-${OBSVersion}-sources",
	[switch]$SkipOBS,
	[string]$VisualStudioVersion="2022",
	[string]$VisualStudioInstallerPath="${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer",
	[string]$DownloadsPath=(New-Object -ComObject Shell.Application).Namespace('shell:Downloads').Self.Path,
	[switch]$Prefetched,
	[string]$InnoSetupDirectory="${env:ProgramFiles}\Inno Setup 7"
)

$VisualStudioInstance=&(Join-Path $VisualStudioInstallerPath -ChildPath "vswhere.exe") -prerelease -format json | ConvertFrom-Json | Where-Object { $_.displayName -match $VisualStudioVersion }
if ($VisualStudioInstance -eq $null) {
	Write-Host "Could not find Visual Studio version ${VisualStudioVersion}" -Foreground Red
	Exit 1
}
Import-Module (Join-Path -Path $VisualStudioInstance.installationPath -ChildPath "Common7" -AdditionalChildPath "Tools", "Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell $VisualStudioInstance.instanceId -DevCmdArguments "-arch=x64" -SkipAutomaticLocation

Get-Command -ErrorAction Stop -Type Application "perl"
Get-Command -ErrorAction Stop -Type Application "nmake"
Get-Command -ErrorAction Stop -Type Application "cmake"
Get-Command -ErrorAction Stop -Type Application "ninja"
Get-Command -ErrorAction Stop -Type Application (Join-Path -Path $InnoSetupDirectory -ChildPath "iscc.exe")

Push-Location -Path $WorkingDirectory
$DirectoryTemplate="qt${QtVersionString}-{0}"
$BuildDirectoryTemplate="${DirectoryTemplate}-build"
$BuildConfigurations=@{
	"debug"=@{
		"BuildDirectory"=($BuildDirectoryTemplate -f "x64d")
		"OutputDirectory"=($DirectoryTemplate -f "x64d")
	}
	"release"=@{
		"BuildDirectory"=($BuildDirectoryTemplate -f "x64")
		"OutputDirectory"=($DirectoryTemplate -f "x64")
	}
}
$CMakePresetsTemplate=Get-Content -Path "CMakePresets.json.in" -Raw
$ConfiguredData=$ExecutionContext.InvokeCommand.ExpandString($CMakePresetsTemplate)
$ConfiguredData | Set-Content -Path "CMakePresets.json" -Encoding utf8

$OBSURL="https://github.com/obsproject/obs-studio/releases/download/${OBSVersion}/OBS-Studio-${OBSVersion}-Sources.tar.gz"
$OBSFileInformation=Invoke-WebRequest -Uri $OBSURL -Method HEAD -UseBasicParsing -ErrorAction Stop
$OBSFileSize=$OBSFileInformation.Headers['Content-Length']
$OBSFilePath=(Join-Path -Path $DownloadsPath -ChildPath "OBS-Studio-${OBSVersion}-Sources.tar.gz")
if (-not $SkipOBS) {
	if (-not $Prefetched) {
		if ($OBSFileSize -ne (Get-Item -Path $OBSFilePath -ErrorAction SilentlyContinue).Length) {
			Invoke-WebRequest -Uri $OBSURL -OutFile $OBSFilePath -UseBasicParsing -ErrorAction Stop
		}
		&tar @("--no-same-permissions","-kxvf",$OBSFilePath)
	}
	Remove-Item -Path $OBSWorkingDirectory -Recurse -Force
	Rename-Item -Path $OBSUnpackedArchiveName -NewName $OBSWorkingDirectory
	Push-Location -Path $OBSWorkingDirectory
	$OutputPath=(Join-Path -Path $WorkingDirectory -ChildPath $OBSWorkingDirectory)
	&cmake @(
		".",
		"-DENABLE_BROWSER=OFF",
		"-DENABLE_FRONTEND=ON",
		"-GVisual Studio $($([version]$VisualStudioInstance.installationVersion).Major) ${VisualStudioVersion}",
		"--preset","windows-x64"
	)
	&cmake @(
		"--build","build_x64",
		"--preset","windows-x64",
		"--config","Release",
		"--parallel"
	)
	&cmake @(
		"--install","build_x64"
		"--prefix",$OutputPath
	)
	$BinaryDirectory="${OutputPath}/bin"
	$ArchitectureSpecificBinaryDirectory="${BinaryDirectory}/64bit"
	Move-Item -Path "${ArchitectureSpecificBinaryDirectory}/*" -Destination $BinaryDirectory -Force
	Remove-Item ${ArchitectureSpecificBinaryDirectory} -Recurse
	$IncludeDirectory="include"
	New-Item -Name $IncludeDirectory -ItemType Directory
	Copy-Item -Path "${OutputPath}/libobs/*.h" -Destination $IncludeDirectory
	Copy-Item -Path "${OutputPath}/frontend/api/obs-frontend-api.h" -Destination $IncludeDirectory
	Pop-Location
}

$OpenSSLURL="https://github.com/openssl/openssl/archive/refs/tags/openssl-${OpenSSLVersion}.zip"
$OpenSSLFileInformation=Invoke-WebRequest -Uri $OpenSSLURL -Method HEAD -UseBasicParsing -ErrorAction Stop
$OpenSSLFileSize=$OpenSSLFileInformation.Headers['Content-Length']
$OpenSSLFilePath=(Join-Path -Path $DownloadsPath -ChildPath "openssl-${OpenSSLVersion}.zip")
$OpenSSLOutputPath=(Join-Path -Path $OpenSSLWorkingDirectory -ChildPath "output")
$OpenSSLCompiler=(Get-Command "jom" -Type Application -ErrorAction SilentlyContinue) ? "jom" : "nmake"

if (-not $SkipOpenSSL) {
	if (-not $Prefetched) {
		if ($OpenSSLFileSize -ne (Get-Item -Path $OpenSSLFilePath -ErrorAction SilentlyContinue).Length) {
			Invoke-WebRequest -Uri $OpenSSLURL -OutFile $OpenSSLFilePath -UseBasicParsing -ErrorAction Stop
		}
		Expand-Archive -Path $OpenSSLFilePath -DestinationPath $WorkingDirectory 2>$null
	}
	Push-Location -Path $OpenSSLWorkingDirectory
	$Arguments=@(
		"Configure",
		"--prefix=${OpenSSLOutputPath}",
		"--openssldir=${OpenSSLOutputPath}",
		"VC-WIN64A"
	)
	if ($OpenSSLCompiler -eq "jom") {
		$Arguments+="/FS"
	}
	&"perl" $Arguments
	&$OpenSSLCompiler
	&"nmake" "install"
	Pop-Location
	$InstallerTemplate=Get-Content -Path "setup.in" -Raw
	$InstallerData=$ExecutionContext.InvokeCommand.ExpandString($InstallerTemplate)
	$InstallerData | Set-Content -Path "setup.iss.in" -Encoding utf8
}

$QtVersion=[version]$QtVersionString
$QtSourceArchive="qt-everywhere-src-${QtVersionString}.zip"
$QtSourceArchivePath=(Join-Path -Path $DownloadsPath -ChildPath $QtSourceArchive)
$QtSourceURL="https://download.qt.io/official_releases/qt/{0}/{1}/single/${QtSourceArchive}" -f $QtVersion.ToString(2), $QtVersionString
$QtSourceArchiveInformation=Invoke-WebRequest -Uri $QtSourceURL -Method HEAD -UseBasicParsing -ErrorAction Stop
$QtSourceArchiveSize=$QtSourceArchiveInformation.Headers['Content-Length']
if (-not $Prefetched) {
	if ($QtSourceArchiveSize -ne (Get-Item -Path $QtSourceArchivePath -ErrorAction SilentlyContinue).Length) {
		Invoke-WebRequest -Uri $QtSourceURL -OutFile $QtSourceArchivePath -UseBasicParsing -ErrorAction Stop
	}
	Expand-Archive -Path $QtSourceArchivePath -DestinationPath $WorkingDirectory 2>$null
}
foreach ($Configuration in $BuildConfigurations.Keys) {
	$BuildDirectory=$BuildConfigurations[$Configuration]["BuildDirectory"]
	New-Item -Name $BuildDirectory -ItemType Directory
	Push-Location -Path $BuildDirectory
	$OutputPath=(Join-Path -Path $WorkingDirectory -ChildPath $BuildConfigurations[$Configuration]["OutputDirectory"])
	$Arguments=@(
		"-prefix", $OutputPath,
		"-opensource",
		"-${Configuration}",
		"-no-pch",
		"-opengl", "dynamic",
		"-confirm-license",
		"-openssl-linked",
		"-verbose",
		"-nomake", "examples",
		"-nomake", "tests",
		"-no-feature-accessibility",
		"-no-feature-androiddeployqt",
		"-no-feature-appstore-compliant",
		"-no-feature-assistant",
		"-no-feature-bluetooth",
		"-no-feature-cups",
		"-no-feature-designer",
		"-no-feature-fullqthelp",
		"-no-feature-gestures",
		"-no-feature-modbus-serialport",
		"-no-feature-printdialog",
		"-no-feature-printpreviewdialog",
		"-no-feature-printpreviewwidget",
		"-no-feature-printsupport",
		"-no-feature-protobufquick",
		"-no-feature-qdoc",
		"-no-feature-qdoc_coverage",
		"-no-feature-qmake",
		"-no-feature-qml-animation",
		"-no-feature-qml-debug",
		"-no-feature-qml-delegate-model",
		"-no-feature-qml-itemmodel",
		"-no-feature-qml-jit",
		"-no-feature-qml-labs",
		"-no-feature-qml-list-model",
		"-no-feature-qml-locale",
		"-no-feature-qml-network",
		"-no-feature-qml-object-model",
		"-no-feature-qml-preview",
		"-no-feature-qml-profiler",
		"-no-feature-qml-sfpm-model",
		"-no-feature-qml-ssl",
		"-no-feature-qml-table-model",
		"-no-feature-qml-tree-model",
		"-no-feature-qml-type-loader-thread",
		"-no-feature-qml-worker-script",
		"-no-feature-qml-xml-http-request",
		"-no-feature-qml-xmllistmodel",
		"-no-feature-qmlcontextpropertydump",
		"-no-feature-qtattributionsscanner",
		"-no-feature-qtplugininfo",
		"-no-feature-quick-animatedimage",
		"-no-feature-quick-canvas",
		"-no-feature-quick-designer",
		"-no-feature-quick-dialogs",
		"-no-feature-quick-draganddrop",
		"-no-feature-quick-flipable",
		"-no-feature-quick-gridview",
		"-no-feature-quick-listview",
		"-no-feature-quick-particles",
		"-no-feature-quick-path",
		"-no-feature-quick-pathview",
		"-no-feature-quick-pixmap-cache-threaded-download",
		"-no-feature-quick-positioners",
		"-no-feature-quick-repeater",
		"-no-feature-quick-shadereffect",
		"-no-feature-quick-sprite",
		"-no-feature-quick-tableview",
		"-no-feature-quick-treeview",
		"-no-feature-quick-vectorimage",
		"-no-feature-quickcontrols2-fluentwinui3",
		"-no-feature-quickcontrols2-fusion",
		"-no-feature-quickcontrols2-imagine",
		"-no-feature-quickcontrols2-ios",
		"-no-feature-quickcontrols2-macos",
		"-no-feature-quickcontrols2-material",
		"-no-feature-quickcontrols2-stylekit",
		"-no-feature-quickcontrols2-universal",
		"-no-feature-quickcontrols2-windows",
		"-no-feature-quicktemplates2-calendar",
		"-no-feature-quicktemplates2-container",
		"-no-feature-quicktemplates2-hover",
		"-no-feature-quicktemplates2-multitouch",
		"-no-feature-sql",
		"-no-feature-sqlmodel",
		"-no-feature-statemachine-qml",
		"-no-feature-textodfwriter",
		"-no-feature-tuiotouch",
		"-no-feature-wasm-exceptions",
		"-no-feature-wasm-jspi",
		"-no-feature-wasm-simd128",
		"-no-feature-wasmdeployqt",
		"-no-feature-websockets-qml",
		"-skip", "qtlottie",
		"-skip", "qtspeech",
		"-skip", "qtvirtualkeyboard",
		"-skip", "qtwebview",
		"-skip", "qtopcua",
		"-skip", "qtwebengine"
	)
	$Arguments+=@("--", "-DOPENSSL_ROOT_DIR=${OpenSSLOutputPath}")
	&"..\${QtUnpackedArchiveName}\configure" $Arguments
	cmake @("--build", ".", "--parallel")
	&cmake @("--install", ".")
	Pop-Location
	Remove-Item -Path $BuildDirectory -Recurse -Force
	Copy-Item -Path "${OpenSSLOutputPath}/bin/libcrypto-$($([version]$OpenSSLVersion).Major)-x64.dll" -Destination "${OutputPath}/bin/" -ErrorAction Stop
	Copy-Item -Path "${OpenSSLOutputPath}/bin/libssl-$($([version]$OpenSSLVersion).Major)-x64.dll" -Destination "${OutputPath}/bin/" -ErrorAction Stop
}

Pop-Location
