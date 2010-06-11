# 
# KDOM IDL parser
#
# Copyright (C) 2005 Nikolas Zimmermann <wildfox@kde.org>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
# 

package IDLStructure;

use strict;

use Class::Struct;

# Used to represent a parsed IDL document
struct( idlDocument => {
    module => '$',   # Module identifier
    classes => '@',  # All parsed interfaces
    fileName => '$'  # file name
});

# Used to represent 'interface' / 'exception' blocks
struct( domClass => {
    name => '$',      # Class identifier (without module)
    parents => '@',      # List of strings
    constants => '@',    # List of 'domConstant'
    functions => '@',    # List of 'domFunction'
    attributes => '@',    # List of 'domAttribute'    
    extendedAttributes => '$', # Extended attributes
});

# Used to represent domClass contents (name of method, signature)
struct( domFunction => {
    signature => '$',    # Return type/Object name/extended attributes
    parameters => '@',    # List of 'domSignature'
    raisesExceptions => '@',  # Possibly raised exceptions.
});

# Used to represent domClass contents (name of attribute, signature)
struct( domAttribute => {
    type => '$',              # Attribute type (including namespace)
    signature => '$',         # Attribute signature
    getterExceptions => '@',  # Possibly raised exceptions.
    setterExceptions => '@',  # Possibly raised exceptions.
});

# Used to represent a map of 'variable name' <-> 'variable type'
struct( domSignature => {
    name => '$',      # Variable name
    type => '$',      # Variable type
    extendedAttributes => '$' # Extended attributes
});

# Used to represent string constants
struct( domConstant => {
    name => '$',      # DOM Constant identifier
    type => '$',      # Type of data
    value => '$',      # Constant value
});

# Helpers
our $idlId = '[a-zA-Z0-9]';        # Generic identifier
our $idlIdNs = '[a-zA-Z0-9:]';      # Generic identifier including namespace
our $idlIdNsList = '[a-zA-Z0-9:,\ ]';  # List of Generic identifiers including namespace

our $idlType = '[a-zA-Z0-9_]';      # Generic type/"value string" identifier
our $idlDataType = '[a-zA-Z0-9\ ]';   # Generic data type identifier

# Magic IDL parsing regular expressions
my $supportedTypes = "((?:unsigned )?(?:int|short|(?:long )?long)|(?:$idlIdNs*))";

# Special IDL notations
our $extendedAttributeSyntax = '\[[^]]*\]'; # Used for extended attributes

# Regular expression based IDL 'syntactical tokenizer' used in the IDLParser
our $moduleSelector = 'module\s*(' . $idlId . '*)\s*{';
our $moduleNSSelector = 'module\s*(' . $idlId . '*)\s*\[ns\s*(' . $idlIdNs . '*)\s*(' . $idlIdNs . '*)\]\s*;';
our $constantSelector = 'const\s*' . $supportedTypes . '\s*(' . $idlType . '*)\s*=\s*(' . $idlType . '*)';
our $raisesSelector = 'raises\s*\((' . $idlIdNsList . '*)\s*\)';
our $getterRaisesSelector = '\bgetter\s+raises\s*\((' . $idlIdNsList . '*)\s*\)';
our $setterRaisesSelector = '\bsetter\s+raises\s*\((' . $idlIdNsList . '*)\s*\)';

our $typeNamespaceSelector = '((?:' . $idlId . '*::)*)\s*(' . $idlDataType . '*)';

our $exceptionSelector = 'exception\s*(' . $idlIdNs . '*)\s*([a-zA-Z\s{;]*};)';
our $exceptionSubSelector = '{\s*' . $supportedTypes . '\s*(' . $idlType . '*)\s*;\s*}';

our $interfaceSelector = 'interface\s*((?:' . $extendedAttributeSyntax . ' )?)(' . $idlIdNs . '*)\s*(?::(\s*[^{]*))?{([a-zA-Z0-9_=\s(),;:\[\]&\|]*)';
our $interfaceMethodSelector = '\s*((?:' . $extendedAttributeSyntax . ' )?)' . $supportedTypes . '\s*(' . $idlIdNs . '*)\s*\(\s*([a-zA-Z0-9:\s,=\[\]]*)';
our $interfaceParameterSelector = 'in\s*((?:' . $extendedAttributeSyntax . ' )?)' . $supportedTypes . '\s*(' . $idlIdNs . '*)';

our $interfaceAttributeSelector = '\s*(readonly attribute|attribute)\s*(' . $extendedAttributeSyntax . ' )?' . $supportedTypes . '\s*(' . $idlType . '*)';

1;
