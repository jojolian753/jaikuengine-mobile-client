# IDE_VC7.PM
#
# Copyright (c) 1999 Symbian Ltd.  All rights reserved.
#


# Makmake-module for creating MAKMAKE Microsoft Visual Studio .net IDE makefiles shared by windows platform modules

package Ide_vc7;

# declare variables global for module
my $BaseAddressFlag;
my @CppCall;
my %IdeBlds=();
my %PrjHdrs=();
my @Win32LibList=();
my $Win32Resrc;

my $SupText='';


require Exporter;
@ISA=qw(Exporter);

@EXPORT=qw(
	PMHelp_Mmp

	PMCheckPlatformL

	PMPlatProcessMmp

	PMStartBldList
		PMBld
	PMEndBldList
	PMStartSrcList
			PMResrcBld
			PMDoc
			PMStartSrc
			PMSrcDepend
	PMEndSrcList
);

use Winutl;

sub PMHelp_Mmp {
# get the windows help from WINUTL
	&Winutl_Help_Mmp;
}

sub PMCheckPlatformL {
	if ((&main::Plat eq 'TOOLS') and (&main::BasicTrgType ne 'EXE')) {
		die "Can't specify anything but EXE targettypes for this platform\n";
	}
}

sub PMPlatProcessMmp (@) {

	# get WINUTL to do the windows mmp file processing
	&Winutl_DoMmp(\@_, $ENV{INCLUDE});
	$BaseAddressFlag=&Winutl_BaseAddress;
	$BaseAddressFlag=~s/^(.+$)$/ \/base:\"$1\"/o;
	@Win32LibList=&Winutl_Win32LibList;
	$Win32Resrc=&Winutl_Win32Resrc;
}

sub PMStartBldList($) {
	my ($makecmd) = @_;
	die "Cannot generate $makecmd makefiles\n" if ($makecmd ne "nmake");
	my $BaseTrg=&main::BaseTrg;
	my @BldList=&main::BldList;
	my $BldPath=&main::BldPath;
	my $ChopDataPath=&main::Path_Chop(&main::DataPath);
	my $ChopTrgPath=&main::Path_Chop(&main::TrgPath);
	my $DefFile=&main::DefFile;
	my $BasicTrgType=&main::BasicTrgType;
	my $LibPath=&main::LibPath;
	my $LinkPath=&main::LinkPath;
	my $Plat=&main::Plat;
	my $RelPath=&main::RelPath;
	my $Trg=&main::Trg;
	my $StatLinkPath=&main::StatLinkPath;
	my $TrgType=&main::TrgType;


	# set up global IDE builds variable
	%IdeBlds= (
		UREL=> "$BaseTrg - Win32 Release WINS",
		UDEB=> "$BaseTrg - Win32 Debug WINS",
	);
#tools hack
	if (&main::Plat eq 'TOOLS') {
		%IdeBlds= (
			REL=> "$BaseTrg - Win32 Release",
			DEB=> "$BaseTrg - Win32 Debug",
		);
	}
	

#	Start the supplementary makefile

	$SupText.=join('',
		"\n",
#			need to set path here because MSDEV might be set up so that
#			it does not take account of the PATH environment variable
		"PATH=$ENV{PATH}\n",
		"\n",
		"# EPOC DEFINITIONS\n",
		"\n",
		"EPOCBLD = $BldPath #\n",
		"EPOCTRG = $RelPath #\n",
		"EPOCLIB = $LibPath #\n",
		"EPOCLINK = $LinkPath #\n",
		"EPOCSTATLINK = $StatLinkPath #\n",
		"\n"
	);
			
	if ($BasicTrgType eq 'DLL') {
		foreach (@BldList) {
			$SupText.=join('',
				"EPOCBLD$_ = \$(EPOCBLD)$_\n",
				"EPOCTRG$_ = \$(EPOCTRG)$_\n",
				"EPOCLIB$_ = \$(EPOCLIB)UDEB\n",
				"EPOCLINK$_ = \$(EPOCLINK)UDEB\n",
				"EPOCSTATLINK$_ = \$(EPOCSTATLINK)$_\n",
				"\n"
			);
		}
		$SupText.="\nTRGDIR = ";
		if ($Plat!~/^WINC$/o && $ChopTrgPath) {	# target path not allowed under WINC
			$SupText.=$ChopTrgPath;
		}
		else {
			$SupText.=".\\";
		}
		$SupText.="\n\nDATADIR = $ChopDataPath\n\n";


#		commands for creating the import library
		$SupText.="LIBRARY :";
		if ($DefFile) {
			unless (&main::ExportUnfrozen) {
				if (-e $DefFile) { # effectively "if project frozen ..."
					$SupText.=" \"\$(EPOCLIB)UDEB\\$BaseTrg.LIB\"\n";
				}
				else {
					$SupText.=join('',
						"\n",
						"\t\@echo WARNING: Not attempting to create \"\$(EPOCLIB)UDEB\\$BaseTrg.LIB\".\n",
						"\t\@echo When exports are frozen in \"$DefFile\", regenerate Makefile.\n"
					);
				}
			}
			else {
				$SupText.=join('',
					"\n",
					"\t\@echo Not attempting to create \"\$(EPOCLIB)UDEB\\$BaseTrg.LIB\"\n",
					"\t\@echo from frozen .DEF file, since EXPORTUNFROZEN specified.\n"
				);
			}
			$SupText.=join('',
				"\n",
				"\n",
				"# REAL TARGET - IMPORT LIBRARY\n",
				"\n",
				"\"\$(EPOCLIB)UDEB\\$BaseTrg.LIB\" : \"$DefFile\" MAKEWORKLIBRARY\n",
				"\tlib.exe /nologo /machine:i386 /nodefaultlib /name:\"$Trg\" /def:\"$DefFile\" /out:\"\$(EPOCLIB)UDEB\\$BaseTrg.LIB\"\n",
				"\tdel \"\$(EPOCLIB)UDEB\\$BaseTrg.exp\"\n"
			);
		}
		$SupText.=join('',
			"\n",
			"\n",
			"MAKEWORKLIBRARY : \"${LibPath}UDEB\"\n",
			"\n",
			"\"${LibPath}UDEB\" :\n"
		);
		$SupText.="\t\@perl -S emkdir.pl \"${LibPath}UDEB\"\n\n\n";
	}

#create the .VCPROJ file 
	
	&main::Output(
		"<?xml version=\"1.0\" encoding=\"Windows-1252\"?>\n",
		"<VisualStudioProject\n",
		"	ProjectType=\"Visual C++\"\n",
		"	Version=\"7.10\"\n",
		"	Name=\"$BaseTrg\"\n",
		"	SccProjectName=\"\"\n",
		"	SccLocalPath=\"\">\n",
		"	<Platforms>\n",
		"		<Platform\n",
		"			Name=\"Win32\"/>\n",
		"	</Platforms>\n",
		"	<Configurations>\n"
	);
}

sub PMBld {

	my @BaseSrcList=&main::BaseSrcList;
	my $BaseTrg=&main::BaseTrg;
	my $Bld=&main::Bld;
	my @BldList=&main::BldList;
	my $ChopBldPath=&main::Path_Chop(&main::BldPath);
	my $ChopLinkPath=&main::Path_Chop(&main::LinkPath);
	my $ChopRelPath=&main::Path_Chop(&main::RelPath);
	my $ChopStatLinkPath=&main::Path_Chop(&main::StatLinkPath);
	my @ChopSysIncPaths=&main::Path_Chop(&main::SysIncPaths);
	my @ChopUserIncPaths=&main::Path_Chop(&main::UserIncPaths);
	my $DefFile=&main::DefFile;
	my $FirstLib=&main::FirstLib;
	my $BasicTrgType=&main::BasicTrgType;
	my @MacroList=&main::MacroList();
	my @VariantMacroList = &main::VariantMacroList();
	my @LibList;
	my $PathBaseDsp=&main::MakeFilePath.&main::BaseMak;
	my @StatLibList=&main::StatLibList;
	my $Trg=&main::Trg;
	my $TrgPath=&main::TrgPath;
	my $TrgType=&main::TrgType;


	if ($Bld =~ /DEB/) {
		@LibList=&main::DebugLibList;
	} else {
		@LibList=&main::LibList;
	}

	my $NotUseWin32LibsFlag="";
# tools hack in line below
	unless ( ($BaseTrg=~/tracealloc/i) or (&main::Plat eq 'TOOLS') ) {
		#die "XXX";
		$NotUseWin32LibsFlag=" /X"; # this flag suppresses searching of the standard directories for header files
	} else {
		$NotUseWin32LibsFlag="";
	}

	
	#&main::Output(
	#	
	#	"		<Configuration\n"
	#);

	if ($Bld=~/REL$/o) {
	
		&main::Output(
			"		<Configuration\n",
			"			Name=\"Release Armi|Win32\"\n",
			"			OutputDirectory=\"Release\"\n",
			"			IntermediateDirectory=\"Release\"\n",
			"			ConfigurationType=\"0\">\n",
			"			<Tool\n",
			"				Name=\"VCNMakeTool\"\n",
			"				BuildCommandLine=\"cmd /c bldmake bldfiles
									   cmd /c abld build armi urel\"\n",
			"				ReBuildCommandLine=\"abld build armi urel\"\n",
			"				CleanCommandLine=\"abld reallyclean armi urel\"\n",
			"				Output=\"$BaseTrg\"/>\n",
			"		</Configuration>"
		);
		
		&main::Output(
			"		<Configuration\n",
			"			Name=\"Release Thumb|Win32\"\n",
			"			OutputDirectory=\"Release\"\n",
			"			IntermediateDirectory=\"Release\"\n",
			"			ConfigurationType=\"0\">\n",
			"			<Tool\n",
			"				Name=\"VCNMakeTool\"\n",
			"				BuildCommandLine=\"cmd /c bldmake bldfiles
									   cmd /c abld build thumb urel\"\n",
			"				ReBuildCommandLine=\"abld build thumb urel\"\n",
			"				CleanCommandLine=\"abld reallyclean thumb urel\"\n",
			"				Output=\"$BaseTrg\"/>\n",
			"		</Configuration>"
		);
	
		&main::Output(
			"		<Configuration\n",
			"			Name=\"Release WINS|Win32\"\n",
			"			OutputDirectory=\"$ChopRelPath\"\n",
			"			IntermediateDirectory=\"$ChopBldPath\"\n"
		);
		if ($BasicTrgType=~/^DLL$/o) {
			&main::Output(
			"			ConfigurationType=\"2\"\n"
			);
		}else {
			&main::Output(
			"			ConfigurationType=\"1\"\n"
			);
		}
		&main::Output(
			"			UseOfMFC=\"0\"\n",
			"			ATLMinimizesCRunTimeLibraryUsage=\"FALSE\"\n",
			"			CharacterSet=\"1\">\n"
		);
		&main::Output(
			"			<Tool\n",
			"				Name=\"VCCLCompilerTool\"\n",
			"				AdditionalOptions=\"/QIfist\"\n",
			"				Optimization=\"1\"\n",
			"				InlineFunctionExpansion=\"1\"\n",
			"				ImproveFloatingPointConsistency=\"TRUE\"\n",
			"				AdditionalIncludeDirectories=\""
		);
	}
	elsif ($Bld=~/DEB$/o) {
		&main::Output(
			"		<Configuration\n",
			"			Name=\"Debug WINS|Win32\"\n",
			"			OutputDirectory=\"$ChopRelPath\"\n",
			"			IntermediateDirectory=\"$ChopBldPath\"\n"
		);
		if ($BasicTrgType=~/^DLL$/o) {
			&main::Output(
			"			ConfigurationType=\"2\"\n"
			);
		}else {
			&main::Output(
			"			ConfigurationType=\"1\"\n"
			);
		}
		&main::Output(
			"			UseOfMFC=\"0\"\n",
			"			ATLMinimizesCRunTimeLibraryUsage=\"FALSE\"\n",
			"			CharacterSet=\"1\">\n"
		);
		
		#Compiler tool section
		&main::Output(
			"			<Tool\n",
			"				Name=\"VCCLCompilerTool\"\n",
			"				AdditionalOptions=\"/QIfist\"\n",
			"				Optimization=\"0\"\n",
			"				AdditionalIncludeDirectories=\""
		);
		
	}
	
	
	my @incList=();
	my @tmpIncList=();
	my $candidate;
	my $tmpCand;
	my $count;
	my $count2=0;
	INCLOOP: foreach $candidate (@ChopUserIncPaths,@ChopSysIncPaths) {
		my $accepted;	# remove duplicates
		foreach $accepted (@incList) {
			if ($candidate eq $accepted) {
				next INCLOOP;
			}
		}
		push @incList, $candidate;
		push @tmpIncList, $candidate;
	}
	$count = @tmpIncList;
	foreach $tmpCand (@tmpIncList) {

		&main::Output(
			"$tmpCand"
		);
		$count2++;

		if($count2<$count) {
				&main::Output(
								","
				);
		}

	}
	&main::Output(
		"\"\n"
	);
	
	if ($Bld=~/REL$/o) {
		&main::Output(
			"			PreprocessorDefinitions=\""
		);
	}
	elsif ($Bld=~/DEB$/o) {
		&main::Output(
			"			PreprocessorDefinitions=\""
		);
	}
	
	$count = @MacroList;
	$count2 =0;
	
	foreach (@MacroList) {
		&main::Output(
			" $_"
		);
		
		$count2++;

		if($count2<$count) {
			&main::Output(
				";"
			);
		}
		
	}
	
	&main::Output(
		"\"\n"
	);
	
	if ($Bld=~/REL$/o) {
		&main::Output(
			"			IgnoreStandardIncludePath=\"TRUE\"\n",
			"			StringPooling=\"TRUE\"\n",
			"			MinimalRebuild=\"TRUE\"\n",
			"			EnableFunctionLevelLinking=\"TRUE\"\n",
			"			ExceptionHandling=\"FALSE\"\n",
		);
		if ($BasicTrgType=~/^DLL$/o) {
			&main::Output(
			"			RuntimeLibrary=\"2\"\n"
			);
		}else {
			&main::Output(
			"			RuntimeLibrary=\"4\"\n"
			);
		}
		&main::Output(
			"			StructMemberAlignment=\"3\"\n",
			"			BufferSecurityCheck=\"FALSE\"\n",
			"			PrecompiledHeaderFile=\"",$ChopBldPath,"/",$BaseTrg,".pch\"\n",
			"			AssemblerListingLocation=\"$ChopBldPath/\"\n",
			"			ObjectFile=\"$ChopBldPath/\"\n",
			"			ProgramDataBaseFileName=\"",$ChopBldPath,"/","\"\n", #\"",$ChopRelPath\\$BaseTrg,".pdb"\"\n",
			"			WarningLevel=\"4\"\n",
			"			SuppressStartupBanner=\"TRUE\"\n",
			"			CompileAs=\"0\"/>\n"
		);
		
		#Custom Build tool section
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCCustomBuildTool\"/>\n"
		);
		
		#Linker tool section
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCLinkerTool\"\n",
			#"			AdditionalOptions=\"/WARN:3\"\n",
			"			AdditionalDependencies="
		);
		

	}
	elsif ($Bld=~/DEB$/o) {
		&main::Output(
			"			IgnoreStandardIncludePath=\"TRUE\"\n",
			"			StringPooling=\"TRUE\"\n",
			"			ExceptionHandling=\"FALSE\"\n"
		);
		if ($BasicTrgType=~/^DLL$/o) {
			&main::Output(
			"			RuntimeLibrary=\"3\"\n"
			);
		}else {
			&main::Output(
			"			RuntimeLibrary=\"5\"\n"
			);
		}
		
		&main::Output(
			"			StructMemberAlignment=\"3\"\n",
			"			BufferSecurityCheck=\"FALSE\"\n",
			"			PrecompiledHeaderFile=\"",$ChopBldPath,"/",$BaseTrg,".pch\"\n",
			"			AssemblerListingLocation=\"$ChopBldPath/\"\n",
			"			ObjectFile=\"$ChopBldPath/\"\n",
			"			ProgramDataBaseFileName=\"$ChopRelPath\\$TrgPath$BaseTrg",".pdb\"\n", #\"$ChopRelPath/\"\n",
			"			BrowseInformation=\"1\"\n",
			"			WarningLevel=\"4\"\n",
			"			SuppressStartupBanner=\"TRUE\"\n",
			"			DebugInformationFormat=\"4\"\n",
			"			CompileAs=\"0\"/>\n"
		);
		#Custom Build tool section
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCCustomBuildTool\"/>\n"
		);

		#Linker tool section
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCLinkerTool\"\n",
			#"			AdditionalOptions=\"/WARN:3\"\n",
			"			AdditionalDependencies="
		);
	}
	
	
	unless (&main::Plat eq 'TOOLS') {
		&main::Output(
			" \"&quot;$ChopStatLinkPath\\$FirstLib&quot;"
		);
	}

	foreach (@Win32LibList) {
		&main::Output(
			" &quot;",lc $_,"&quot;"
		);
	}
	foreach (@StatLibList) {
		&main::Output(
			" &quot;$ChopStatLinkPath\\",lc $_,"&quot;"
		);
	}
	foreach (@LibList) {
		&main::Output(
			" &quot;$ChopLinkPath\\",lc $_,"&quot;"
		);

	}
	if (0 && $BasicTrgType=~/^DLL$/o) {
		&main::Output(
			" &quot;$ChopBldPath\\$BaseTrg.exp&quot;"
		);
	}
	&main::Output("\"\n");
	
	&main::Output(
		 "			OutputFile=\"$ChopRelPath\\$TrgPath$Trg\"\n",
		 "			LinkIncremental=\"2\"\n",
		 "			SuppressStartupBanner=\"TRUE\"\n",
		 "			IgnoreAllDefaultLibraries=\"TRUE\"\n",
		 "			ForceSymbolReferences=\""
	);
	
	#Tools hack
	unless (&main::Plat eq 'TOOLS') {
		if ($BasicTrgType=~/^DLL$/o) {
			&main::Output(
				'?_E32Dll@@YGHPAXI0@Z',"\"\n"
			);
		}
		elsif ($BasicTrgType=~/^EXE$/o) {
			&main::Output(
				'?_E32Startup@@YGXXZ',"\"\n"
			);
		}
	
	}
	#tools hack ends
	
	if ($Bld=~/DEB$/o) {
	
		&main::Output(
			"		GenerateDebugInformation=\"TRUE\"\n"
		);
	}
	if ($BasicTrgType=~/^DLL/o) {
		if ( ($BaseTrg=~/factory/i || $BaseTrg=~/plugin/i) && $DefFile) {
			&main::Output("ModuleDefinitionFile='&quot;" . $DefFile . "&quot;'\n");
		} else {
			my $Ordinal;
			my $Num=1;
			my $exp="";
			foreach $Ordinal (&main::Exports) {
				$exp .= $Ordinal . " @ " . $Num . " NONAME;\n";
				$Num++;
			}
			unless($exp eq "") {
				my $file=&main::MakeFilePath . "EXPORTS.DEF";
				open (DEFS, ">" . $file ) || die "cannot open $file";
				print DEFS "EXPORTS\n";
				print DEFS $exp;
				close(DEFS);
				&main::Output("ModuleDefinitionFile='&quot;" . $file . "&quot;'\n");
			}
		}
	}
	
	if ($Bld=~/DEB$/o) {
		&main::Output(
			"			ProgramDatabaseFile=\"$ChopRelPath\\$TrgPath$BaseTrg.pdb\"\n",
			"			SubSystem=\"2\"\n",
			"			EntryPointSymbol="
		);
	}elsif($Bld=~/REL$/o)
	{
		&main::Output(
				"			ProgramDatabaseFile=\"$ChopRelPath/$BaseTrg.pdb\"\n",
				"			SubSystem=\"2\"\n",
				"			EntryPointSymbol="
		);
	}
	if ($BasicTrgType=~/^EXE$/o) {
	# tools hack
		unless (&main::Plat eq 'TOOLS') {
			if ($BaseTrg !~ /tracealloc/i) {
				&main::Output(
					"\"_E32Startup\"\n"
				);
			} else {
				&main::Output(
					"\"_TraceStartup\"\n"
				);
			}
		}
	# tools hack end
	}
	elsif ($BasicTrgType=~/^DLL$/o) {
		&main::Output(
			"\"_E32Dll\"\n"
		);
	}
	
	if ($TrgType=~/DLL/o) {
		if ($Bld=~/REL$/o){
			&main::Output(
				"			ImportLibrary=\"$ChopRelPath/$BaseTrg.lib\""
			);
		}
		elsif ($Bld=~/DEB$/o) {
			&main::Output(
				"			ImportLibrary=\"$ChopRelPath/$BaseTrg.lib\""
			);	
		}
	}
	&main::Output("/>\n");
	
	#################
	#MIDLTool
	&main::Output(
		"		<Tool\n",
		"			Name=\"VCMIDLTool\"\n"
	);
	
	if ($Bld=~/DEB$/o) {

		&main::Output(
		"			PreprocessorDefinitions=\"_DEBUG\"\n"
		);
	}
	
	&main::Output(
		"			MkTypLibCompatible=\"TRUE\"\n",
		"			SuppressStartupBanner=\"TRUE\"\n",
		"			TargetEnvironment=\"1\"\n",
		"			TypeLibraryName=\"$ChopRelPath/$BaseTrg.tlb\"\n",
		"			HeaderFileName=\"\"/>\n"
	);
	
	if (0 && $BasicTrgType=~/^DLL$/o) {
		##################
		#PostBuildEvenTool
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCPostBuildEventTool\"\n",
			"			CommandLine=\"nmake -nologo -f &quot;${PathBaseDsp}.SUP.MAKE&quot; POSTBUILD$Bld\"/>\n",
		);

		###################
		#PreBuildEventTool
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCPreBuildEventTool\"/>\n"
		);

		#################
		#PreLinkEventTool
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCPreLinkEventTool\"\n",
			"			CommandLine=\"echo Doing first-stage link by name\n", 
			"\tnmake -nologo -f &quot;${PathBaseDsp}.SUP.MAKE&quot; PRELINK$Bld\n",
			"\tif errorlevel 1 nmake -nologo -f &quot;${PathBaseDsp}.SUP.MAKE&quot; STOPLINK$Bld\n\"/>\n"	
		);
	}else
	{
		##################
		#PostBuildEvenTool
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCPostBuildEventTool\"/>\n"
		);

		###################
		#PreBuildEventTool
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCPreBuildEventTool\"/>\n"
		);

		#################
		#PreLinkEventTool
		&main::Output(
			"		<Tool\n",
			"			Name=\"VCPreLinkEventTool\"/>\n"	
		);
	}
	#####################
	#ResourceCompilerTool
	&main::Output(
		"		<Tool\n",
		"			Name=\"VCResourceCompilerTool\"\n",
		"			PreprocessorDefinitions=\""
	);
	
	if ($Bld=~/REL$/o) {
		&main::Output(
		"NDEBUG\"\n"
		);
	}
	elsif ($Bld=~/DEB$/o) {
		&main::Output(
		"_DEBUG\"\n"
		);
	}
	
	&main::Output(
		"			Culture=\"2057\"/>\n"
	);
	
	
	#############################
	#WebServiceProxyGeneratorTool
	&main::Output(
		"		<Tool\n",
		"			Name=\"VCWebServiceProxyGeneratorTool\"/>\n"
	);	

	#####################
	#XMLDataGeneratorTool
	&main::Output(
		"		<Tool\n",
		"			Name=\"VCXMLDataGeneratorTool\"/>\n"
	);	
	
	##################
	#WebDeploymentTool
	&main::Output(
		"		<Tool\n",
		"			Name=\"VCWebDeploymentTool\"/>\n"
	);
	
	############################
	#ManagedWrapperGeneratorTool
	&main::Output(
		"		<Tool\n",
		"			Name=\"VCManagedWrapperGeneratorTool\"/>\n"
	);
	
	#####################################
	#AuxiliaryManagedWrapperGeneratorTool
	&main::Output(
		"		<Tool\n",
		"			Name=\"VCAuxiliaryManagedWrapperGeneratorTool\"/>\n"
	);
	
	
	&main::Output(
		"	</Configuration>\n"
	);
	
	my @incList=();
	my $candidate;
	INCLOOP: foreach $candidate (@ChopUserIncPaths,@ChopSysIncPaths) {
		my $accepted;	# remove duplicates
		foreach $accepted (@incList) {
			if ($candidate eq $accepted) {
				next INCLOOP;
			}
		}
		push @incList, $candidate;

	}
	foreach (@MacroList) {

	}
	#}

	if ($BasicTrgType=~/^DLL$/o) {

#		append to the supplementary makefile for each build
		$SupText.=join('',
			"# BUILD - $Bld\n",
			"\n",
			"LIBS="
		);
		foreach (@StatLibList) {
			$SupText.=" \\\n\t\"\$(EPOCSTATLINK$Bld)\\$_\"";
		}
		foreach (@LibList) {
			$SupText.=" \\\n\t\"\$(EPOCLINK$Bld)\\$_\"";
		}
		$SupText.="\n\nLINK_OBJS=";
		foreach (@BaseSrcList) {
			$SupText.=" \\\n\t\"\$(EPOCBLD$Bld)\\$_.obj\"";
		}
		if ($Win32Resrc) {
			$SupText.=" \\\n\t\"\$(EPOCBLD$Bld)\\".&main::Path_Split('Base',$Win32Resrc).".res\"";
		}
		$SupText.="\n\nSTAGE1_LINK_FLAGS=\"\$(EPOCSTATLINK$Bld)\\$FirstLib\"";
		foreach (@Win32LibList) {
			$SupText.=' ';
			$SupText.=lc $_;
		}
		$SupText.=" \\\n \$(LIBS) /nologo$BaseAddressFlag /entry:\"_E32Dll\" /subsystem:windows /dll";
		if ($Bld=~/DEB$/o) {
			$SupText.=' /debug';
		}
		$SupText.=" \\\n /incremental:no /machine:IX86 /nodefaultlib /include:\"".'?_E32Dll@@YGHPAXI0@Z'."\" /out:\"\$(EPOCBLD$Bld)\\$Trg\" /WARN:3\n\n";

		$SupText.="PRELINK$Bld : \$(LINK_OBJS)";
		if (-e $DefFile) { # effectively "if project frozen ..."
			$SupText.=" \"$DefFile\"";
		}
# tools hack
		unless (&main::Plat eq 'TOOLS') {
			$SupText.=" \"\$(EPOCSTATLINK$Bld)\\$FirstLib\"";
		}
# tools hack end
		$SupText.=" \$(LIBS)\n";

#		Link by name first time round for dlls
		$SupText.=join('',
			"\tlink.exe \@<<\n",
			"\t\t\$(STAGE1_LINK_FLAGS) \$(LINK_OBJS)\n",
			"<<\n",
			"\tdel \"\$(EPOCBLD$Bld)\\$Trg\"\n",
			"\tdel \"\$(EPOCBLD$Bld)\\$BaseTrg.exp\"\n"
		);

#		Generate an export info file
		$SupText.=join('',
			"\tdumpbin /exports /out:\"\$(EPOCBLD$Bld)\\$BaseTrg.inf\" \"\$(EPOCBLD$Bld)\\$BaseTrg.lib\"\n",
			"\tdel \"\$(EPOCBLD$Bld)\\$BaseTrg.lib\"\n"
		);

#		call makedef to reorder the export information
#		call perl on the script here so nmake will die if there are errors - this doesn't happen if calling perl in a batch file
		$SupText.="\tperl -S makedef.pl -Inffile \"\$(EPOCBLD$Bld)\\$BaseTrg.inf\"";
		if (-e $DefFile) { # effectively "if project frozen ..."
			$SupText.=" -Frzfile \"$DefFile\"";
		}
		else { # freeze ordinals, a maximum of 2, for polymorphic dlls
			my $Ordinal;
			my $Num=1;
			foreach $Ordinal (&main::Exports) {
#				replace "$" with "$$" so that NMAKE doesn't think there's a macro in the function name
				$Ordinal=~s-\$-\$\$-go;
				$SupText.=" -$Num $Ordinal";
				$Num++;
			}
		}
		$SupText.=join('',
			" \"\$(EPOCBLD)$BaseTrg.def\" \n",
			"\tdel \"\$(EPOCBLD$Bld)\\$BaseTrg.inf\"\n"
		);

		# create the export object from the .DEF file
		$SupText.="\tlib.exe  /nologo /machine:i386 /nodefaultlib /name:\"$Trg\" /def:\"\$(EPOCBLD)$BaseTrg.def\" /out:\"\$(EPOCBLD$Bld)\\$BaseTrg.lib\"\n";
		if (&main::ExportUnfrozen) {
			$SupText.="\tcopy \"\$(EPOCBLD$Bld)\\$BaseTrg.lib\" \"\$(EPOCLIB)UDEB\\$BaseTrg.LIB\"\n";
		}
		$SupText.=join('',
			"\tdel \"\$(EPOCBLD$Bld)\\$BaseTrg.lib\"\n",
			"\t\@echo First-stage link successful\n",
			"\n",
			"\n",
			"STOPLINK$Bld : DELEXPOBJ$Bld\n",
			"\t\@echo Stopped the build by removing the export object,\n",
			"\t\@echo if present, because the pre-link stage failed\n",
			"\n",
			"\n",
			"POSTBUILD$Bld : DELEXPOBJ$Bld"
		);
		if ($DefFile and not &main::ExportUnfrozen) {
			$SupText.=" LIBRARY";
		}
		$SupText.=join('',
			"\n",
			"\n",
			"\n",
			"DELEXPOBJ$Bld :\n",
			"\tif exist \"\$(EPOCBLD$Bld)\\$BaseTrg.exp\" del \"\$(EPOCBLD$Bld)\\$BaseTrg.exp\"\n",
			"\n",
			"\n",
			"\n"
		);
	}

	&main::Output(
		"\n"
	);
}

sub PMEndBldList {

	# CLOSE THE !IF ... !ENDIF LOOP
	#------------------------------
#	&main::Output(
#		"!ENDIF \n",
#		"\n"
#	);
}

sub PMStartSrcList {

	my @BldList=&main::BldList;
	&main::Output(
		"	</Configurations>\n",
		"	<References>\n",
		"	</References>\n"
	);
	&main::Output(
	"	<Files>\n",
	"		<Filter\n",
	"			Name=\"Source Files\"\n",
	"			Filter=\"cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90\">\n",
	);
}

sub PMResrcBld {

	my $ResourceRef=&main::ResourceRef;
	my $Resrc=ucfirst lc &main::Path_Split('File', $$ResourceRef{Source});
	my $BaseResrc=&main::Path_Split('Base', $$ResourceRef{Source});
	my $SrcPath=&main::Path_Split('Path', $$ResourceRef{Source});
	my $TrgPath=&main::Path_Split('Path', $$ResourceRef{Trg});
	my @LangList=($$ResourceRef{Lang});
	my $inputpath="$SrcPath$Resrc";
	
	
	my @BldList=&main::BldList;
	my $ChopBldPath=&main::Path_Chop(&main::BldPath);
	my @ChopSysIncPaths=&main::Path_Chop(&main::SysIncPaths);
	my @ChopUserIncPaths=&main::Path_Chop(&main::UserIncPaths);
	my $DataPath=&main::DataPath;
	my @DepList=&main::DepList;
	my $PathBaseDsp=&main::MakeFilePath.&main::BaseMak;
	my $PerlLibPath=&main::PerlLibPath;
	my $RelPath=&main::RelPath;
	my $ChopSrcPath=&main::Path_Chop($SrcPath);

	my $ResrcHdr=join '', &main::EPOCIncPath(), $BaseResrc, '.RSG';
	
	
	$SupText.="DEPEND=";
	my $Dep;
	
	foreach $Dep (@DepList) {
		$SupText.=" \\\n\t\"$Dep\"";
	}
		
	$SupText.="\n\n";

	my $Bld;
	&main::Output(
				"		<File \n",
				"			RelativePath=\"$inputpath\">\n",
		);
	foreach $Bld (@BldList) {
		my $ResrcTrgFullName="$RelPath$Bld\\$TrgPath$BaseResrc.r";
		
		&main::Output(
			"			<FileConfiguration\n"
		);
		
		
		
		if ($Bld =~ /DEB/) {
			&main::Output(
			"				Name=\"Debug WINS|Win32\">\n"

			);
		}else
		{
			&main::Output(
			"				Name=\"Release WINS|Win32\">\n"

			);
		}
		
		&main::Output(
			"				<Tool\n",
			"					Name=\"VCCustomBuildTool\"\n",
			"					Description=\"Building resources from $Resrc\"\n",
							
		);
		
		&main::Output(
			"					CommandLine=\"nmake -nologo -f &quot;${PathBaseDsp}.SUP.MAKE&quot; &quot;$ResrcTrgFullName&quot;\"\n",
			"					AdditionalDependencies=\""
		);
		
		foreach $Dep (@DepList) {
			&main::Output(
				"$Dep;"
			);
		}
		&main::Output(
				"\"\n"
			);
		$SupText.="\"$ResrcTrgFullName\" :";
		my $Lang;
		foreach $Lang (@LangList) {
			if ($Lang eq 'SC') {
#				hack to put dummy file in dependency list
				$SupText.=" \"$ResrcTrgFullName$Lang.dummy\"";
				next;
			}
			$SupText.=" \"$ResrcTrgFullName$Lang\"";
		}
		$SupText.="\n\n";
		foreach $Lang (@LangList) {
#			hack to put dummy file in dependency list
			if ($Lang eq 'SC') {
				$SupText.="\"$ResrcTrgFullName$Lang.dummy\"";
			}
			else {
				$SupText.="\"$ResrcTrgFullName$Lang\"";
			}
			$SupText.=" : \"$SrcPath$Resrc\" \$(DEPEND)\n";
			$SupText.="\tperl -S epocrc.pl -I \"$ChopSrcPath\"";
			foreach (@ChopUserIncPaths) {
				$SupText.=" -I \"$_\"";
			}
			$SupText.=" -I-";
			foreach (@ChopSysIncPaths) {
				$SupText.=" -I \"$_\"";
			}
			$SupText.=" -DLANGUAGE_$Lang -u \"$SrcPath$Resrc\"";
			$SupText.=" -o\"$ResrcTrgFullName$Lang\" -h\"$SrcPath$BaseResrc.rs~\" -t\"\$(EPOCBLD$Bld)\"\n";
# 			hack because if a .RSC file is output then VC5 tries to link it to the main target as a Win32 resource file
			if ($Lang eq 'SC') {
				$SupText.="\techo this is a dummy output file > \"$ResrcTrgFullName$Lang.dummy\"\n";
			}
			$SupText.=join('',
				"\tperl -S ecopyfile.pl \"$SrcPath$BaseResrc.rs~\" \"$ResrcHdr\"\n",
				"\tdel \"$SrcPath$BaseResrc.rs~\"\n",
				"\n"
			);
		}

		foreach $Lang (@LangList) {
#			hack again to avoid VC5 linking the resource
			my $TmpLang=$Lang;
			if ($TmpLang eq 'SC') {
				$TmpLang.='.dummy';
			}
			&main::Output(
				"			Outputs=\"$ResrcTrgFullName$TmpLang\"/>\n"
			);
		}
		&main::Output(
			"			</FileConfiguration>\n"

		);
	}
	&main::Output(
			"		</File>\n"
		);
}

sub PMDoc {    

	my $SrcPath=&main::SrcPath;

	&main::Output(
		"		<File\n",
		"			RelativePath=\"",$SrcPath,ucfirst lc &main::Doc,"\">\n",
		"			<FileConfiguration\n",
		"				Name=\"Release WINS|Win32\"\n",
		"				ExcludedFromBuild=\"TRUE\">\n",
		"				<Tool\n",
		"					Name=\"VCCustomBuildTool\"/>\n",
		"			</FileConfiguration>\n",
		"			<FileConfiguration\n",
		"				Name=\"Debug WINS|Win32\"\n",
		"				ExcludedFromBuild=\"TRUE\">\n",
		"				<Tool\n",
		"					Name=\"VCCustomBuildTool\"/>\n",
		"			</FileConfiguration>\n"
	);

}

sub PMStartSrc {    
	
	my @MacroList=&main::MacroList();
	my @VariantMacroList = &main::VariantMacroList();
	
	&main::Output("		<File\n");
	&main::Output("			RelativePath=\"",&main::SrcPath,ucfirst lc &main::Src,"\">\n");
	&main::Output("			<FileConfiguration\n",
		      "				Name=\"Release WINS|Win32\">\n");
	&main::Output("				<Tool\n",
		      "					Name=\"VCCLCompilerTool\"\n",
		      "					Optimization=\"1\"\n",
		      "					AdditionalIncludeDirectories=\"\"\n",
		      "					PreprocessorDefinitions=\"");
	foreach (@MacroList) {
			&main::Output(
				" $_;"
			);
	}
	&main::Output("NDEBUG;_UNICODE;\$(NoInherit)\"/>\n"); #"NDEBUG" and "_UNICODE" are hardcoded, needs to be changed later. 
	&main::Output("			</FileConfiguration>\n");
	&main::Output("			<FileConfiguration\n",
		      "				Name=\"Debug WINS|Win32\">\n");
	&main::Output("				<Tool\n",
		      "					Name=\"VCCLCompilerTool\"\n",
		      "					Optimization=\"0\"\n",
		      "					AdditionalIncludeDirectories=\"\"\n",
		      "					PreprocessorDefinitions=\"");
	foreach (@MacroList) {
			&main::Output(
				" $_;"
			);
	}
	&main::Output("_DEBUG;_UNICODE;\$(NoInherit)\"\n",
		      "					BrowseInformation=\"1\"/>\n"
	       ); #"_DEBUG" and "_UNICODE" are hardcoded, needs to be changed later.
	&main::Output("			</FileConfiguration>\n");
	&main::Output("		</File>\n");
	
}

sub PMSrcDepend {

	# Generate user header list for this src, merge with list for all sources
	foreach (&main::DepList) {
		$PrjHdrs{$_}='unusedval';
	}
}

sub PMEndSrcList {
	
	my $BaseDsp=&main::BaseMak;
	my $PathBaseDsp=&main::MakeFilePath.$BaseDsp;
	
	&main::Output(
		"		</File>\n",
		"	</Filter>\n"
	);
	
	&main::Output(
		"	<Filter\n",
		"		Name=\"Resource Files\"\n",
		"		Filter=\"ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe\">\n",
		
	);
	if ($Win32Resrc) {
		
		&main::Output(
			"		<File\n",
			"			RelativePath=\"",ucfirst lc $Win32Resrc,"\">\n",
			"		</File>\n"
		);
		
		
		# Generate user header list for this src, merge with list for all sources
		foreach (&main::Deps_GenDependsL($Win32Resrc)) {
			$PrjHdrs{$_}='unusedval';
		}
	}

	&main::Output(
	"	</Filter>\n"
	);

	# Use the global %PrjHdrs Hash to produce user header listing
	&main::Output(
		"	<Filter\n",
		"		Name=\"Header Files\"\n",
		"		Filter=\"h;hpp;hxx;hm;inl;fi;fd\">\n"
		
	);
	foreach (keys %PrjHdrs) {
		&main::Output(
			"		<File\n",
			"			RelativePath=\"",&main::Path_Split('Path',$_),ucfirst lc &main::Path_Split('File',$_),"\">\n",
			"		</File>\n"
		) unless ($_=~/\.c(pp)?$/i);
	}
	&main::Output(
		"	</Filter>\n"
	);
	
	&main::Output(
		"	</Files>\n",
		"	<Globals>\n",
		"	</Globals>\n",
		"</VisualStudioProject>\n"
	);
	

	&main::Path_DelFiles("$PathBaseDsp.MAK","$PathBaseDsp.MDP","$PathBaseDsp.NCB","$PathBaseDsp.OPT","$PathBaseDsp.PLG");

#	Add target to supplementary makefile to recreate the workspace
#
#	This target is intended for use as a custom tool within the MSVC IDE, for regenerating
#	workspace once the .MMP file has been edited within the IDE.  To install the target as
#	a custom tool in the IDE, select Tools->Customise...->Tools, and choose a name for the
#	tool, e.g. "Recreate Workspace".  Next, type "nmake.exe" as the command and
#	"-nologo -f $(WkspDir)\$(WkspName).sup.make recreateworkspace" as the program arguments.
#	Leave the "initial directory" field blank, and tick the "Close window on exiting" checkbox.
#	Having edited the .MMP file for a project, select the new tool from the tools menu to
#	recreate the workspace.  If the commands have run correctly, you will be prompted to
#	reload the workspace.
	$SupText.=join('',
		"\n",
		"RECREATEWORKSPACE :\n",
		'	cd ', &main::Path_Chop(&main::Path_WorkPath), "\n",
		'	perl -S makmake.pl -D ', &main::MmpFile, ' ', &main::PlatName, "\n",
		"\n"
	);

#	Create the supplementary makefile
	
	&main::CreateExtraFile(&main::MakeFilePath.&main::BaseMak.'.SUP.MAKE', $SupText);


# Creates .SLN file
# At this moment uses fixed project GUID.
	my $SlnText=join(
		"\n",
		"Microsoft Visual Studio Solution File, Format Version 8.00",
		"Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\")=\"$BaseDsp\",\"$BaseDsp.vcproj\", \"{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}\"",
		"	ProjectSection(ProjectDependencies) = postProject",
		"	EndProjectSection",
		"EndProject",
		"Global",
		"	GlobalSection(SolutionConfiguration) = preSolution",
		"		Debug WINS = Debug WINS",
		"		Release WINS = Release WINS",
		"		Release Armi = Release Armi",
		"		Release Thumb = Release Thumb",
		"	EndGlobalSection",
		"	GlobalSection(ProjectConfiguration) = postSolution",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Debug WINS.ActiveCfg = Debug WINS|Win32",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Debug WINS.Build.0 = Debug WINS|Win32",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Release WINS.ActiveCfg = Release WINS|Win32",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Release WINS.Build.0 = Release WINS|Win32",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Release Armi.ActiveCfg = Release Armi|Win32",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Release Armi.Build.0 = Release Armi|Win32",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Release Thumb.ActiveCfg = Release Thumb|Win32",
		"		{C2C5885D-96B3-47EF-9571-1C6C56BDE5EF}.Release Thumb.Build.0 = Release Thumb|Win32",
		"	EndGlobalSection",
		"	GlobalSection(ExtensibilityGlobals) = postSolution",
		"	EndGlobalSection",
		"	GlobalSection(ExtensibilityAddIns) = postSolution",
		"	EndGlobalSection",
		"EndGlobal"
	);

	&main::CreateExtraFile("$PathBaseDsp.sln",$SlnText);
}

1;
