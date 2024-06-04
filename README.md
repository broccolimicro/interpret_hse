# libinterpret_hse

This library provides an interpreter for converting from an hse abstract syntax tree to an hse graph.

Dependencies:
 * parse_hse
	 - parse_boolean
		 - parse
			 - common
 * hse
	 - boolean
		 - common
 * interpret_boolean
	 - parse_boolean
		 - parse
			 - common
	 - boolean
		 - common

## License

Licensed by Cornell University under GNU GPL v3.

Written by Ned Bingham.
Copyright © 2020 Cornell University.

Haystack is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Haystack is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License may be found in COPYRIGHT.
Otherwise, see <https://www.gnu.org/licenses/>.
