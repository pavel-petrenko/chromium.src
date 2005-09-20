#
# KDOM IDL parser
#
# Copyright (c) 2005 Nikolas Zimmermann <wildfox@kde.org>
#
package IDLCodeGeneratorEcmaInterface;

my $useModule = "";
my $useModuleNS = "";
my $useOutputDir = "";
my $useLayerOnTop = 0;

my $codeGenerator;

my $IMPL;
my $HEADER;

my @implContent; # .cpp file content
my @headerContent; # .h file content

my @classes; # Helper.
my $headerString; # Helper.

my @neededForwards; # Used to build up include list

# Default .h template
my $headerTemplate = << "EOF";
/*
    This file is part of the KDE project.
    This file has been generated by kdomidl.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
EOF

# Default constructor
sub new
{
	my $object = shift;
	my $reference = { };

	$codeGenerator = shift;
	$useOutputDir = shift;
	my $useDocumentation = shift; # not used here.
	$useLayerOnTop = shift;

	bless($reference, $object);
	return $reference;
}

sub finish
{
	my $object = shift;

	# Commit changes!
	$object->WriteData();
}

# Params: 'idlDocument' struct
sub GenerateModule
{
	my $object = shift;
	my $dataNode = shift;

	# Create module <-> namespace map...
	$codeGenerator->CreateModuleNamespaceHash();

	$useModule = $dataNode->module;

	my %hash = %{$codeGenerator->ModuleNamespaceHash()};
	$useModuleNS = $hash{$useModule};

	# Determine all classes within the project...
	@classes = @{$codeGenerator->AllClasses()};
}

# Params: 'domClass' struct
sub GenerateInterface
{
	my $object = shift;
	my $dataNode = shift;

	$object->WriteData();

	# Start actual generation..
	print "  |  |>  Generating implementation...\n";
	$object->GenerateImplementation($dataNode);

	print "  |  |>  Generating header...\n";
	$object->GenerateHeader($dataNode);

	print " |-\n |\n";
  
	# Open file for writing...
	my $implFileName = $useOutputDir . "/EcmaInterface.cpp";
	my $headerFileName = $useOutputDir . "/EcmaInterface.h";

	open($IMPL, ">$implFileName") || die "Coudln't open file $implFileName";
	open($HEADER, ">$headerFileName") || die "Coudln't open file $headerFileName";
}

sub GenerateHeader
{
	my $object = shift;

	my $dataNode = shift;

	if(!defined($HEADER)) {
		# - Add default header template
		@headerContent = split("\r", $headerTemplate);

		# - Add header protection
		my $printNS = $useModuleNS; $printNS =~ s/\:\:/\_/g;
		push(@headerContent, "\n#ifndef $printNS\_EcmaInterface_H\n");
		push(@headerContent, "#define $printNS\_EcmaInterface_H\n\n");
		push(@headerContent, "#include <kdom/core/CDFInterface.h>\n");

		if($useModuleNS !~ /^KDOM/) {
			push(@headerContent, "#include <kdom/ecma/EcmaInterface.h>\n");
		}

		push(@headerContent, "#include <kdom/core/DocumentImpl.h>\n\n");
		push(@headerContent, "namespace KJS\n{\n    class ObjectImp;\n}\n\n");

		# - Add placeholder to be replaced with needed forwards, later.
		push(@headerContent, "#FIXUP_FORWARDS#");

		# - Add namespace selector(s)
		my @namespaces = @{$codeGenerator->SplitNamespaces($useModuleNS)};

		my $collectedNS = "";
		foreach(@namespaces) {
			$collectedNS .= $_ . "::";

			my $showNS = substr($collectedNS, 0, length($collectedNS) - 2);
			push(@headerContent, "namespace $_\n{\n");
		}
	}

	# - Add class definition...
	my $inheritanceString = "";
	$inheritanceString = " : public KDOM::EcmaInterface" if($useModuleNS !~ /^KDOM/);

	push(@headerContent, "    class EcmaInterface${inheritanceString}\n    {\n    public:\n");
	push(@headerContent, "        EcmaInterface() { }\n");
	push(@headerContent, "        virtual ~EcmaInterface() { }\n\n");
	push(@headerContent, $headerString);
	push(@headerContent, "    };");


	# - Build forward list...
	if($useLayerOnTop eq 0) {
		foreach(@classes) {
			push(@neededForwards, $_ . "Impl");
		}
	}

	# End header...
	my @namespaces = @{$codeGenerator->SplitNamespaces($useModuleNS)};
	my $namespacesMax = @namespaces;

	foreach(@namespaces) {
		push(@headerContent, "\n};\n");
	}

	push(@headerContent, "\n#endif\n");

	# ... and apply fixups!
	my $tempData = join("@", @headerContent);

	my $fixupString = "";

	@neededForwards = sort { length $a <=> length $b } @neededForwards;
	foreach(@neededForwards) {
		my $useClass = $_;

		my @namespaces = @{$codeGenerator->SplitNamespaces($useClass)};
		my $namespaceCount = @namespaces;

		my $i = 0;
		my $collectedNS = "";
		foreach(@namespaces) {
			$i++;
			$collectedNS .= $_ . "::";

			my $showNS = substr($collectedNS, 0, length($collectedNS) - 2);
			if($i < $namespaceCount) { 
				$fixupString .= "namespace $_ { ";
			} else {
				$fixupString .= "class ${_}; ";
				$fixupString .= ("} " x ($i - 1));
				$fixupString .= "\n";
			}
		}
	}

	$fixupString .= "\n" if($fixupString ne "");

	$tempData =~ s/#FIXUP_FORWARDS#/$fixupString/;
	@headerContent = split("@", $tempData);
}

sub GenerateImplementation
{
	my $object = shift;

	my $dataNode = shift;

	my $parentsMax = @{$dataNode->parents};

	my %extractedType = $codeGenerator->ExtractNamespace($dataNode->name, 1, $useModuleNS);
	$extractedType{'type'} .= "Wrapper"; # Postfix the classname...

	my $implClass = $extractedType{'type'};
	$implClass =~ s/Wrapper$/Impl/;

	if(!defined($IMPL)) {
		# - Add default header template
		@implContent = split("\r", $headerTemplate);

		# - Add absolutely needed includes
		if($useLayerOnTop eq 0) {
			push(@implContent, "\n#include <kdom/ecma/EcmaInterface.h>\n\n");
		} elsif($useModule eq "svg") { # Special case for ksvg...
			push(@implContent, "\n#include <ksvg2/ecma/EcmaInterface.h>");
			push(@implContent, "\n#include <kdom/ecma/DOMBridge.h>\n\n");
		}

		# - Add placeholder to be replaced with needed includes, later.
		push(@implContent, "#FIXUP_INCLUDES#");

		# - Add namespace selector(s)
		my @namespaces = @{$codeGenerator->SplitNamespaces($useModuleNS)};

		my $collectedNS = "";
		foreach(@namespaces) {
			$collectedNS .= $_ . "::";

			my $showNS = substr($collectedNS, 0, length($collectedNS) - 2);
			push(@implContent, "using namespace $showNS;\n");
		}
	}

	$headerString = ""; # Cleanup.

	my $showTypeNS = $useModuleNS;
	$showTypeNS =~ s/([A-Z]*)[A-Za-z:]*/$1/g;

	my $useThisClass = 0; # Helper.

	# - Add 'inheritedFoobarCast' functions for all the kdom classes
	#   which are reimplemented within our project (ie. ksvg2/khtml2).
	foreach(@classes) {
		my $class = $_;
		if($class !~ /KDOM/) { # Skip our own base classes.
			next;
		}

		my $include = $class;
		$include =~ s/([a-zA-Z0-9]*::)*//; # Strip namespace.

		if($useLayerOnTop eq 0) {
			$headerString .= "        virtual ${_}Impl *inherited${include}" .
							 "Cast(const KJS::ObjectImp *bridge);\n";

			push(@implContent, "\n${_}Impl *${showTypeNS}::EcmaInterface::inherited" .
							   $include . "Cast(const KJS::ObjectImp *bridge)\n{\n" .
							   "    Q_UNUSED(bridge);\n    return 0;\n}\n");
		} else {
			my @inheritedByClassList = @{$codeGenerator->AllClassesWhichInheritFrom($_)};
			my $inheritedBySize = @inheritedByClassList;

			my $classString = "";
			if($inheritedBySize ne 0) {
				my $used = 0;
				foreach(@inheritedByClassList) {
					my %inheritedType = $codeGenerator->ExtractNamespace($_, 1, $useModuleNS);

					my $type = $inheritedType{'type'};
					my $namespace = $inheritedType{'namespace'};

					if($namespace !~ /^KDOM/) {
						# No duplicated includes...
						my @array1 = grep { /^${type}Impl$/ } @neededForwards;
						my @array2 = grep { /^${type}Wrapper$/ } @neededForwards;
						my $arraySize1 = @array1; my $arraySize2 = @array2;

						push(@neededForwards, $type . "Impl") if($arraySize1 eq 0);
						push(@neededForwards, $type . "Wrapper") if($arraySize2 eq 0);

						$classString .= "    { ${class}Impl *test = ${namespace}::to${type}(0, bridge);" .
										"\n    if(test) return test; }\n\n";
					}
				}
			}

			if($classString ne "") {
				$headerString .= "        ${class}Impl *inherited${include}" .
								 "Cast(const KJS::ObjectImp *bridge);\n";

				push(@implContent, "\n${class}Impl *${showTypeNS}::EcmaInterface::inherited" .
								   $include . "Cast(const KJS::ObjectImp *bridge)\n{\n" .
								   $classString . "    return 0;\n}\n");
			}
		}
	}

	# ... and apply fixups!
	my $tempData = join("@", @implContent);

	my $fixupString = "";

	foreach(@neededForwards) {
		$_ =~ s/([a-zA-Z0-9]*::)*//; # Strip namespace.
	}

	@neededForwards = sort { length $a <=> length $b } @neededForwards;
	foreach(@neededForwards) {
		my $useClass = $_;

		if($useClass =~ /^.*AbsImpl$/) { # Special cases for SVG generation!
			$useClass =~ s/AbsImpl/Impl/; # it's 'SVGPathSegArcImpl' not 'SVGPathSegArcAbsImpl'
		} elsif($useClass =~ /^.*RelImpl$/) {
			$useClass =~ s/RelImpl/Impl/; # it's 'SVGPathSegArcImpl' not 'SVGPathSegArcRelImpl'
		}

		$fixupString .= "#include \"${useClass}.h\"\n";
	}

	$fixupString .= "\n" if($fixupString ne "");

	$tempData =~ s/#FIXUP_INCLUDES#/$fixupString/;
	@implContent = split("@", $tempData);

	@neededForwards = ();
}

# Internal helper
sub WriteData
{
	if(defined($IMPL)) {
		# Write content to file.
		print $IMPL @implContent;
		close($IMPL);
		undef($IMPL);

		@implContent = "";
	}

	if(defined($HEADER)) {
		# Write content to file.
		print $HEADER @headerContent;
		close($HEADER);
		undef($HEADER);

		@headerContent = "";
	}
}

1;
