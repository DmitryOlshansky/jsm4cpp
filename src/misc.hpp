#pragma once

template<class It>
bool myEqual(It abeg, It aend, It bbeg, It bend){
	It i=abeg, j=bbeg;
	for(; i!=aend && j!=bend; i++, j++){
		if(*i != *j)
			return false;
	}
	return i == aend && j == bend;
}