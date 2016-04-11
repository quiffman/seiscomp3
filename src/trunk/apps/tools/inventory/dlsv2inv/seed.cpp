/*==============================================================================

Purpose:    parsing seed files to file SeisComp database

Language:   C++, ANSI standard.

Author:     Peter de Boer

Revision:   0.1 initial  10-01-2008

==============================================================================*/
#include "seed.h"
using namespace std;

/****************************************************************************************************************************
* Function:     FromJulianDay												    *
* Parameters:   date	- date in year/julian_day format								    *
* Returns:      date in year/month/day format										    *
****************************************************************************************************************************/
string FromJulianDay(string date)
{
	long jaar, maand, dag;
	int jan, feb, mrt, apr, mei, jun, jul, aug, sep, okt, nov, dec;
	int pos1=0, pos2;

	jaar = FromString<long>(SplitString(date, ',', pos1, pos2));
	pos1 = ++pos2;
	dag = FromString<long>(SplitString(date, ',', pos1, pos2));

	// start initializing days of each month
	if((jaar % 4)==0)
	{
		if(jaar % 400)
			feb = 29;
		else if(jaar % 100)
			feb = 28;
		else
			feb = 29;
	}
	else
		feb = 28;

	jan = mrt = mei = jul = aug = okt = dec = 31;
	apr = jun = sep = nov = 30;

	if((dag -= jan)<=0)
	{
		maand = 1;
		dag += jan;
	}
	else if((dag -= feb)<=0)
	{
		maand = 2;
		dag += feb;
	}
	else if((dag -= mrt)<=0)
	{
		maand = 3;
		dag += mrt;
	}
	else if((dag -= apr)<=0)
	{
		maand = 4;
		dag += apr;
	}
	else if((dag -= mei)<=0)
	{
		maand = 5;
		dag += mei;
	}
	else if((dag -= jun)<=0)
	{
		maand = 6;
		dag += jun;
	}
	else if((dag -= jul)<=0)
	{
		maand = 7;
		dag += jul;
	}
	else if((dag -= aug)<=0)
	{
		maand = 8;
		dag += aug;
	}
	else if((dag -= sep)<=0)
	{
		maand = 9;
		dag += sep;
	}
	else if((dag -= okt)<=0)
	{
		maand = 10;
		dag += okt;
	}
	else if((dag -= nov)<=0)
	{
		maand = 11;
		dag += nov;
	}
	else if((dag -= dec)<=0)
	{
		maand = 12;
		dag += dec;
	}
	Date internal_date(jaar, maand, dag);
	stringstream jd;
	jd << internal_date << "T00:00:00.0000Z";
	return jd.str();
}

/****************************************************************************************************************************
* Function:     SetRemains												    *
* Parameters:   record	- remains of the record that has been processed							    *
* Returns:      nothing													    *
* Description:  save the remains of the current record to add to the new one						    *
****************************************************************************************************************************/
void Control::SetRemains(const string& record)
{
	remains_of_record = record;
}

/****************************************************************************************************************************
* Function:     SetRemains												    *
* Parameters:   record	- the buffer that contains the data that has been processed					    *
*		size	- number of bytes to be added to bytes_left							    *
* Returns:      nothing													    *
* Description:  gets the remains from record and saves it in remains_of_record						    *
****************************************************************************************************************************/
void Control::SetRemains(const string& record, int size)
{
	remains_of_record = record.substr(0, (bytes_left+size));
}

/****************************************************************************************************************************
* Function:     GetRemains												    *
* Parameters:   none													    *
* Returns:      the remains of the previous record									    *
****************************************************************************************************************************/
string Control::GetRemains()
{
	return remains_of_record;
}

/****************************************************************************************************************************
* Function:     SetBytesLeft												    *
* Parameters:   blockette_size	- size of the blockette being processed							    *
* Returns:      true if blockette can be processed, false if else							    *
****************************************************************************************************************************/
bool Control::SetBytesLeft(int blockette_size, const string& record) {
	// check if last character is an ~, if so than add 1 to blockette_size
	if ( blockette_size < (int)record.size() && record[blockette_size] == '~' )
		blockette_size++;
	bytes_left -= blockette_size;
	if ( bytes_left >= 0 )
		return true;
	else
		return false;
}

/****************************************************************************************************************************
* Function:     SetBytes												    *
* Parameters:   size	- size to be set										    *
* Returns:      nothing													    *
* Description:  set the variable bytes_left to the initial size of the record that is processed				    *
****************************************************************************************************************************/
void Control::SetBytes(int size)
{
	bytes_left = size;
}

/****************************************************************************************************************************
* Function:     GetBytesLeft												    *
* Parameters:   none													    *
* Returns:      number of bytes that is left in the record								    *
****************************************************************************************************************************/
int Control::GetBytesLeft()
{
	return bytes_left;
}

/****************************************************************************************************************************
 * Function:     ParseVolumeRecord											    *
 * Parameters:   record	- the string that will be parsed								    *
 * Returns:      nothing												    *
 * Description:  splitting the record into defined blockettes								    *
 ****************************************************************************************************************************/
void VolumeIndexControl::ParseVolumeRecord(string record)
{
	unsigned int blockette, size;
	bool proceed = true;

	log = new Logging();
	SetBytes(LRECL-8);
	if(!GetRemains().empty())
	{
		record = GetRemains() + record;
		SetBytes(record.size());
		SetRemains("");
	}

	do
	{
		try
		{
			// get blockette number and size
			blockette = FromString<int>(record.substr(0,3).c_str());
			size = FromString<int>(record.substr(3, 4).c_str());

			// check if the complete blockette is within the current record
			if(SetBytesLeft(size, record))
			{
				switch(blockette)
				{
					case(5):
						fvi.push_back(FieldVolumeIdentifier(record.substr(7, size)));
						break;
					case(8):
						tvi.push_back(TelemetryVolumeIdentifier(record.substr(7, size)));
						break;
					case(10):
						vi.push_back(VolumeIdentifier(record.substr(7, size)));
						break;
					case(11):
						vshi.push_back(VolumeStationHeaderIndex(record.substr(7, size)));
						break;
					case(12):
						vtsi.push_back(VolumeTimeSpanIndex(record.substr(7, size)));
						break;
					default:
						proceed = false;
						break;
				}
				// must check if size of the record is smaller or equal to the size of the last blockette
				// else you'll get an segmentation fault
				if(record.size() <= size)
				{
					proceed = false;
				}
				else
				{
					// check if character on position is an ~,
					// if so than take substring with size+1
					if(record[size] == '~')
						record = record.substr(++size);
					else
						record = record.substr(size);
				}
			}
			else
			{
				SetRemains(record, size);
				proceed = false;
			}
		}
		catch(BadConversion &o)
                {
                        log->write(o.what());
			proceed = false;
                }
		catch(std::out_of_range &o)
                {
                        log->write(string(o.what()) + " 1");
			SetBytes(0);
			SetRemains("");
			proceed = false;
                }

	}while(proceed);
}

/****************************************************************************************************************************
* Function:     EmptyVectors												    *
* Parameters:   none													    *
* Returns:      nothing													    *
* Description:  clear all vectors after the data has been written to inventory						    *
****************************************************************************************************************************/
void VolumeIndexControl::EmptyVectors()
{
	if(!fvi.empty())
		fvi.clear();
	if(!tvi.empty())
		tvi.clear();
	if(!vi.empty())
		vi.clear();
	if(!vshi.empty())
		vshi.clear();
	if(!vtsi.empty())
		vtsi.clear();
}

/****************************************************************************************************************************
* Function:     FieldVolumeIdentifier											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the FieldVolumeIdentifier									    *
****************************************************************************************************************************/
FieldVolumeIdentifier::FieldVolumeIdentifier(string record)
{
	version_of_format = FromString<float>(record.substr(0, 4));
	logical_record_length = FromString<int>(record.substr(4, 2));
	int pos1=6, pos2;
	beginning_of_volume = SplitString(record, SEED_SEPARATOR, pos1, pos2);
}

/****************************************************************************************************************************
* Function:     TelemetryVolumeIdentifier										    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the TelemetryVolumeIdentifier								    *
****************************************************************************************************************************/
TelemetryVolumeIdentifier::TelemetryVolumeIdentifier(string record)
{
	version_of_format = FromString<float>(record.substr(0, 4));
	logical_record_length = FromString<int>(record.substr(4, 2));
	station_identifier = record.substr(6, 5);
	location_identifier = record.substr(11, 2);
	channel_identifier = record.substr(13, 3);
	int pos1=16, pos2;
	beginning_of_volume = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	end_of_volume = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	station_information_effective_date = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	channel_information_effective_date = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	network_code = record.substr(++pos2, 2);
}

/****************************************************************************************************************************
* Function:     VolumeIdentifier											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the VolumeIdentifier										    *
****************************************************************************************************************************/
VolumeIdentifier::VolumeIdentifier(string record)
{
	version_of_format = FromString<float>(record.substr(0, 4));
	logical_record_length = FromString<int>(record.substr(4, 2));
	int pos1=6, pos2;
	beginning_time = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	end_time = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	volume_time = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	originating_organization = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	label = SplitString(record, SEED_SEPARATOR, pos1, pos2);
}

/****************************************************************************************************************************
* Function:     VolumeStationHeaderIndex										    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the VolumeStationHeaderIndex									    *
****************************************************************************************************************************/
VolumeStationHeaderIndex::VolumeStationHeaderIndex(string record)
{
	number_of_stations = FromString<int>(record.substr(0, 3));
	int pos1=3, pos2=8;
	for ( int i = 0; i < number_of_stations; ++i ) {
		station_info[record.substr(pos1, 5)] = FromString<int>(record.substr(pos2, 6));
		pos1 += 11;
 		pos2 += 11;
	}
}

/****************************************************************************************************************************
* Function:     VolumeTimeSpanIndex											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the VolumeTimeSpanIndex									    *
****************************************************************************************************************************/
VolumeTimeSpanIndex::VolumeTimeSpanIndex(string record)
{
	number_of_spans = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	TimeSpan ts;
 	ts.beginning_of_span = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	ts.end_of_span = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	ts.sequence_number = FromString<int>(record.substr(++pos2, 6));
	timespans.push_back(ts);
}

/****************************************************************************************************************************
 * Function:     ParseVolumeRecord											    *
 * Parameters:   record	- the string that will be parsed								    *
 * Returns:      nothing												    *
 * Description:  splitting the record into defined blockettes								    *
 ****************************************************************************************************************************/
void AbbreviationDictionaryControl::ParseVolumeRecord(string record)
{
	unsigned int blockette, size, position = 0;
	bool proceed = true;

	log = new Logging();
	SetBytes(LRECL-8);
	if ( !GetRemains().empty() ) {
		record = GetRemains() + record;
		SetBytes(record.size());
		SetRemains("");
	}

	do {
		try {
			blockette = FromString<int>(record.substr(0,3).c_str());
			size = FromString<int>(record.substr(3, 4).c_str());

			if ( SetBytesLeft(size, record) ) {
				switch ( blockette ) {
					case(30):
						dfd.push_back(DataFormatDictionary(record.substr(7, size)));
						break;
					case(31):
						cd.push_back(CommentDescription(record.substr(7, size)));
						break;
					case(32):
						csd.push_back(CitedSourceDictionary(record.substr(7, size)));
						break;
					case(33):
						ga.push_back(GenericAbbreviation(record.substr(7, size)));
						break;
					case(34):
						ua.push_back(UnitsAbbreviations(record.substr(7, size)));
						break;
					case(35):
						bc.push_back(BeamConfiguration(record.substr(7, size)));
						break;
					case(41):
						fird.push_back(FIRDictionary(record.substr(7, size)));
						break;
					case(42):
						rpd.push_back(ResponsePolynomialDictionary(record.substr(7, size)));
						break;
					case(43):
						rpzd.push_back(ResponsePolesZerosDictionary(record.substr(7, size)));
						break;
					case(44):
						rcd.push_back(ResponseCoefficientsDictionary(record.substr(7, size)));
						break;
					case(45):
						rld.push_back(ResponseListDictionary(record.substr(7, size)));
						break;
					case(46):
						grd.push_back(GenericResponseDictionary(record.substr(7, size)));
						break;
					case(47):
						dd.push_back(DecimationDictionary(record.substr(7, size)));
						break;
					case(48):
						csgd.push_back(ChannelSensitivityGainDictionary(record.substr(7, size)));
						break;
					default:
						proceed = false;
						break;
				}
				// must check if size of the record is smaller or equal to the size of the last blockette
				// else you'll get an segmentation fault
				if ( record.size() <= size )
					proceed = false;
				else {
					// check if character on position is an ~,
					// if so than take substring with size+1
					if ( record[size] == '~' )
						record = record.substr(++size);
					else
						record = record.substr(size);
					position += size;
				}
			}
			else {
				SetRemains(record, size);
				proceed = false;
			}
		}
		catch ( BadConversion &o ) {
			log->write(o.what());
			proceed = false;
		}
		catch ( std::out_of_range &o ) {
			log->write(string(o.what()) + " 2");
			SetBytes(0);
			SetRemains("");
			proceed = false;
		}
	}
	while ( proceed );
}

/****************************************************************************************************************************
* Function:     EmptyVectors												    *
* Parameters:   none													    *
* Returns:      nothing													    *
* Description:  clear all vectors after the data has been written to inventory						    *
****************************************************************************************************************************/
void AbbreviationDictionaryControl::EmptyVectors()
{
	if ( !dfd.empty() )
		dfd.clear();
	if ( !cd.empty() )
		cd.clear();
	if ( !csd.empty() )
		csd.clear();
	if ( !ga.empty() )
		ga.clear();
	if ( !ua.empty() )
		ua.clear();
	if ( !bc.empty() )
		bc.clear();
	if ( !fird.empty() )
		fird.clear();
	if ( !rpd.empty() )
		rpd.clear();
	if ( !rpzd.empty() )
		rpzd.clear();
	if ( !rcd.empty() )
		rcd.clear();
	if ( !rld.empty() )
		rld.clear();
	if ( !grd.empty() )
		grd.clear();
	if ( !dd.empty() )
		dd.clear();
	if ( !csgd.empty() )
		csgd.clear();
}

/****************************************************************************************************************************
* Function:     DataFormatDictionary											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the DataFormatDictionary									    *
****************************************************************************************************************************/
DataFormatDictionary::DataFormatDictionary(string record) {
	int pos1=0, pos2;
	short_descriptive_name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	data_format_identifier_code = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	data_family_type = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	number_of_decoder_keys = FromString<int>(record.substr(pos1, 2));
	pos1 += 2;
	for ( int i = 0; i < number_of_decoder_keys; ++i ) {
		decoder_keys.push_back(SplitString(record, SEED_SEPARATOR, pos1, pos2));
		pos1 = ++pos2;
	}
}

/****************************************************************************************************************************
* Function:     CommentDescription											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the CommentDescription									    *
****************************************************************************************************************************/
CommentDescription::CommentDescription(string record) {
	comment_code_key = FromString<int>(record.substr(0, 4));
	comment_class_code = record[4];
	int pos1=5, pos2;
	description_of_comment = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	units = FromString<int>(record.substr(++pos2, 3));
}

/****************************************************************************************************************************
* Function:     CitedSourceDictionary											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the CitedSourceDictionary									    *
****************************************************************************************************************************/
CitedSourceDictionary::CitedSourceDictionary(string record) {
	source_lookup_code = FromString<int>(record.substr(0, 2));
	int pos1=2, pos2;
	name_of_publication = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	date_published = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	publisher_name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
}

/****************************************************************************************************************************
* Function:     GenericAbbreviation											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the GenericAbbreviation									    *
****************************************************************************************************************************/
GenericAbbreviation::GenericAbbreviation(string record) {
	lookup_code = FromString<int>(record.substr(0, 3));
	int pos1=3, pos2;
	description = SplitString(record, SEED_SEPARATOR, pos1, pos2);
}

/****************************************************************************************************************************
* Function:     UnitsAbbreviations											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the UnitsAbbrevations									    *
****************************************************************************************************************************/
UnitsAbbreviations::UnitsAbbreviations(string record) {
	lookup_code = FromString<int>(record.substr(0, 3));
	int pos1=3, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	description = SplitString(record, SEED_SEPARATOR, pos1, pos2);
}

/****************************************************************************************************************************
* Function:     BeamConfiguration											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the BeamConfiguration									    *
****************************************************************************************************************************/
BeamConfiguration::BeamConfiguration(string record) {
	lookup_code = FromString<int>(record.substr(0, 3));
	number_of_components = FromString<int>(record.substr(3, 4));
	int pos1=7;
	Component comp;
	for ( int i = 0; i < number_of_components; ++i ) {
		comp.station = record.substr(pos1, 5);
		pos1 += 5;
		comp.location = record.substr(pos1, 2);
		pos1 += 2;
		comp.channel = record.substr(pos1, 3);
		pos1 += 3;
		comp.subchannel = FromString<int>(record.substr(pos1, 4));
		pos1 += 4;
		comp.component_weight = FromString<int>(record.substr(pos1, 5));
		pos1 += 5;
		components.push_back(comp);
 	}
}

/****************************************************************************************************************************
* Function:     FIRDictionary												    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the FIRDictionary										    *
****************************************************************************************************************************/
FIRDictionary::FIRDictionary(string record) {
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	symmetry_code = record[pos1++];
	signal_in_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	signal_out_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	number_of_factors = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	for ( int i = 0; i < number_of_factors; ++i ) {
		coefficients.push_back(FromString<double>(record.substr(pos1, 14)));
		pos1 += 14;
	}
}

/****************************************************************************************************************************
* Function:     ResponsePolynomialDictionary										    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponsePolynomialDictionary								    *
****************************************************************************************************************************/
ResponsePolynomialDictionary::ResponsePolynomialDictionary(string record) {
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	transfer_function_type  = record[pos1++];
	signal_in_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	signal_out_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	polynomial_approximation_type  = record[pos1++];
	valid_frequency_units  = record[pos1++];
	lower_valid_frequency_bound = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	upper_valid_frequency_bound = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	lower_bound_of_approximation = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	upper_bound_of_approximation = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	maximum_absolute_error = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	number_of_pcoeff = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	Coefficient pc;
	for ( int i = 0; i < number_of_pcoeff; ++i ) {
		pc.coefficient = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		pc.error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		polynomial_coefficients.push_back(pc);
	}
}

/****************************************************************************************************************************
* Function:     ResponsePolesZerosDictionary										    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponsePolesZerosDictionary								    *
****************************************************************************************************************************/
ResponsePolesZerosDictionary::ResponsePolesZerosDictionary(string record) {
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	response_type  = record[pos1++];
	signal_in_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	signal_out_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	ao_normalization_factor = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	normalization_frequency = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	number_of_zeros = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	Zeros cz;
	for ( int i = 0; i < number_of_zeros; ++i ) {
		cz.real_zero = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cz.imaginary_zero = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cz.real_zero_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cz.imaginary_zero_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		complex_zeros.push_back(cz);
	}
	number_of_poles = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	Poles cp;
	for ( int i = 0; i < number_of_poles; ++i ) {
		cp.real_pole = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cp.imaginary_pole = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cp.real_pole_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cp.imaginary_pole_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		complex_poles.push_back(cp);
	}
}

/****************************************************************************************************************************
* Function:     ResponseCoefficientsDictionary										    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponseCoefficientsDictionary								    *
****************************************************************************************************************************/
ResponseCoefficientsDictionary::ResponseCoefficientsDictionary(string record) {
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	response_type  = record[pos1++];
	signal_in_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	signal_out_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	number_of_numerators = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	Coefficient coeff;
	for ( int i = 0; i < number_of_numerators; ++i ) {
		coeff.coefficient = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		coeff.error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		numerators.push_back(coeff);
	}
	number_of_denominators = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	for ( int i = 0; i < number_of_denominators; ++i ) {
		coeff.coefficient = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		coeff.error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		denominators.push_back(coeff);
	}
}

/****************************************************************************************************************************
* Function:     ResponseListDictionary											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponseListDictionary									    *
****************************************************************************************************************************/
ResponseListDictionary::ResponseListDictionary(string record) {
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	signal_in_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	signal_out_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	number_of_responses = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	ListedResponses lr;
	for ( int i = 0; i < number_of_responses; ++i ) {
		lr.frequency = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.amplitude= FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.amplitude_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.phase_angle = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.phase_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		responses_listed.push_back(lr);
	}
}

/****************************************************************************************************************************
* Function:     GenericResponseDictionary										    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the GenericResponseDictionary								    *
****************************************************************************************************************************/
GenericResponseDictionary::GenericResponseDictionary(string record) {
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	signal_in_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	signal_out_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	number_of_corners = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	CornerList cl;
	for(int i=0; i<number_of_corners; i++)
	{
		cl.frequency = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cl.slope = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		corners_listed.push_back(cl);
	}
}

/****************************************************************************************************************************
* Function:     DecimationDictionary											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the DecimationDictionary									    *
****************************************************************************************************************************/
DecimationDictionary::DecimationDictionary(string record)
{
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	input_sample_rate = FromString<double>(record.substr(pos1, 10));
	pos1 += 10;
	decimation_factor = FromString<int>(record.substr(pos1, 5));
	pos1 += 5;
	decimation_offset = FromString<int>(record.substr(pos1, 5));
	pos1 += 5;
	estimated_delay = FromString<double>(record.substr(pos1, 11));
	pos1 += 11;
	correction_applied = FromString<double>(record.substr(pos1, 11));
	pos1 += 11;
}

/****************************************************************************************************************************
* Function:     ChannelSensitivityDictionary										    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ChannelSensitivityDictionary								    *
****************************************************************************************************************************/
ChannelSensitivityGainDictionary::ChannelSensitivityGainDictionary(string record)
{
	lookup_key = FromString<int>(record.substr(0, 4));
	int pos1=4, pos2;
	name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	sensitivity_gain = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	frequency = FromString<double>(record.substr(pos1, 12));
	pos1 += 12;
	number_of_history_values = FromString<int>(record.substr(pos1, 2));
	pos1 += 2;
	HistoryValues hv;
	for(int i=0; i<number_of_history_values; i++)
	{
		hv.sensitivity_for_calibration = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		hv.frequency_of_calibration_sensitivity = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		hv.time_of_calibration = SplitString(record, SEED_SEPARATOR, pos1, pos2);
		pos1 = ++pos2;
		history_values.push_back(hv);
	}
}

/****************************************************************************************************************************
 * Function:     ParseVolumeRecord											    *
 * Parameters:   record	- the string that will be parsed								    *
 * Returns:      nothing												    *
 * Description:  splitting the record into defined blockettes								    *
 ****************************************************************************************************************************/
void StationControl::ParseVolumeRecord(string record)
{
	unsigned int blockette, size, position = 0;
	bool proceed = true;

	log = new Logging();
	SetBytes(record.size());
	if(!GetRemains().empty())
	{
		record = GetRemains() + record;
 		SetBytes(record.size());
		SetRemains("");
	}

	do
	{
		try
		{
			int begin = 0;
			blockette = 0;
			while(blockette < 50 || blockette > 62)
			{
				blockette = FromString<int>(record.substr(begin++, 3).c_str());
				if ( begin >= (int)record.size() ) {
					SetBytes(0);
					SetRemains("");
					proceed = false;
					return;
				}
			}

			if ( !begin )
				begin += 3;
			else
				begin += 2;

			if ( !si.empty() ) {
				if(blockette < 52)
					eos = (int)si.size()-1;
				if(eos < (int)si.size() && !si[eos].ci.empty())
					eoc = si[eos].ci.size()-1;
			}

			size = FromString<int>(record.substr(begin, 4).c_str());
			if ( SetBytesLeft(size, record) ) {
				begin += 4;

				if ( blockette > 50 && eos == -1 )
					log->write("received blockette " + ToString(blockette) + ", but no station info available");
				else if ( blockette > 52 && eoc == -1 )
					log->write("received blockette " + ToString(blockette) + ", but no channel info available");
				else {
					switch ( blockette ) {
						case(50):
							si.push_back(StationIdentifier(record.substr(begin, size)));
							break;
						case(51):
							si[eos].sc.push_back(Comment(record.substr(begin, size)));
							break;
						case(52):
							eos = AddChannelToStation(ChannelIdentifier(record.substr(begin, size)));
	//						si[eos].ci.push_back(ChannelIdentifier(record.substr(begin, size)));
							break;
						case(53):
							si[eos].ci[eoc].rpz.push_back(ResponsePolesZeros(record.substr(begin, size)));
							break;
						case(54):
							si[eos].ci[eoc].rc.push_back(ResponseCoefficients(record.substr(begin, size)));
							break;
						case(55):
							si[eos].ci[eoc].rl.push_back(ResponseList(record.substr(begin, size)));
							break;
						case(56):
							si[eos].ci[eoc].gr.push_back(GenericResponse(record.substr(begin, size)));
							break;
						case(57):
							si[eos].ci[eoc].dec.push_back(Decimation(record.substr(begin, size)));
							break;
						case(58):
							si[eos].ci[eoc].csg.push_back(ChannelSensitivityGain(record.substr(begin, size)));
							break;
						case(59):
							si[eos].ci[eoc].cc.push_back(Comment(record.substr(begin, size)));
							break;
						case(60):
							si[eos].ci[eoc].rr.push_back(ResponseReference(record.substr(begin, size)));
							break;
						case(61):
							si[eos].ci[eoc].firr.push_back(FIRResponse(record.substr(begin, size)));
							break;
						case(62):
							si[eos].ci[eoc].rp.push_back(ResponsePolynomial(record.substr(begin, size)));
							break;
						default:
							proceed = false;
							break;
					}
				}
				// must check if size of the record is smaller or equal to the size of the last blockette
				// else you'll get an segmentation fault
				if(record.size() <= size)
				{
					proceed = false;
				}
				else
				{
					// check if character on position is an ~,
					// if so than take substring with size+1
					if(begin!=7)
					{
						size += (begin-7);
					}
					if(record[size] == '~')
						record = record.substr(++size);
					else
						record = record.substr(size);
					position += size;
				}
			}
			else
			{
				SetRemains(record, size);
				proceed = false;
			}
		}
		catch(BadConversion &o)
		{
			log->write(o.what());
			proceed = false;
		}
		catch(std::out_of_range &o)
		{
			log->write(string(o.what()) + " 3");
			SetBytes(0);
			SetRemains("");
			proceed = false;
		}
	}while(proceed);
}

/****************************************************************************************************************************
* Function:     AddChannelToStation											    *
* Parameters:   chan	ChannelIdentifier										    *
* Returns:      number of station in vector										    *
* Description:  						    *
****************************************************************************************************************************/
int StationControl::AddChannelToStation(ChannelIdentifier chan)
{
	for(int i=si.size()-1; i>-1; i--)
	{
		if(chan.GetEndDate().empty() || chan.GetEndDate() > si[i].GetStartDate())
		{
			if(si[i].GetEndDate().empty() || si[i].GetEndDate() > chan.GetStartDate())
			{
				si[i].ci.push_back(chan);
				return i;
			}
		}
	}

	si[si.size()-1].ci.push_back(chan);
	return si.size()-1;
}

/****************************************************************************************************************************
* Function:     EmptyVectors												    *
* Parameters:   none													    *
* Returns:      nothing													    *
* Description:  clear all vectors after the data has been written to inventory						    *
****************************************************************************************************************************/
void StationControl::EmptyVectors()
{
	if(!si.empty())
	{
		for(size_t i=0; i<si.size(); i++)
			si[i].EmptyVectors();
		si.clear();
	}
}

/****************************************************************************************************************************
* Function:     StationIdentifier											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the StationIdentifier									    *
****************************************************************************************************************************/
StationIdentifier::StationIdentifier(string record)
{
	station_call_letters = record.substr(0, 5);
	latitude = FromString<double>(record.substr(5, 10));
	longitude = FromString<double>(record.substr(15, 11));
	elevation = FromString<double>(record.substr(26, 7));
	number_of_channels = FromString<int>(record.substr(33, 4));
	number_of_station_comments = FromString<int>(record.substr(37, 3));
	int pos1=40, pos2;
	site_name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	network_identifier_code = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	word_order = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	short_order = FromString<int>(record.substr(pos1, 2));
	pos1 += 2;
	start_date = SplitString(record, SEED_SEPARATOR, pos1, pos2);
//	if(start_date != "")
//		start_date = FromJulianDay(start_date);
	pos1 = ++pos2;
	end_date = SplitString(record, SEED_SEPARATOR, pos1, pos2);
//	if(end_date != "")
//		end_date = FromJulianDay(end_date);
	pos1 = ++pos2;
	update_flag = record[pos1++];
	network_code = record.substr(pos1, 2);
	pos1 = 0;
	city = SplitString(site_name, ',', pos1, pos2);
	if(pos2 < (int)site_name.size())
	{
		pos1 = ++pos2;
		country = SplitString(site_name, ',', pos1, pos2);
		if(pos2 < (int)site_name.size())
		{
			province = country;
			pos1 = ++pos2;
			country = SplitString(site_name, ',', pos1, pos2);
		}
	}
}

/****************************************************************************************************************************
* Function:     EmptyVectors												    *
* Parameters:   none													    *
* Returns:      nothing													    *
* Description:  clear all vectors after the data has been written to inventory						    *
****************************************************************************************************************************/
void StationIdentifier::EmptyVectors()
{
	if(!sc.empty())
		sc.clear();
	if(!ci.empty())
	{
		for(size_t i=0; i<ci.size(); i++)
			ci[i].EmptyVectors();
		ci.clear();
	}
}

/****************************************************************************************************************************
* Function:     Comment													    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the Comment											    *
****************************************************************************************************************************/
Comment::Comment(string record)
{
	int pos1=0, pos2;
	beginning_effective_time = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	end_effective_time = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	comment_code_key = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	comment_level = FromString<int>(record.substr(pos1, 6));
}

/****************************************************************************************************************************
* Function:     ChannelIdentifier											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ChannelIdentifier									    *
****************************************************************************************************************************/
ChannelIdentifier::ChannelIdentifier(string record)
{
	location = record.substr(0, 2);
	channel = record.substr(2, 3);
	subchannel = FromString<int>(record.substr(5, 4));
	instrument = FromString<int>(record.substr(9, 3));
	int pos1=12, pos2;
	optional_comment = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	units_of_signal_response = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	units_of_calibration_input = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	latitude = FromString<double>(record.substr(pos1, 10));
	pos1 += 10;
	longitude = FromString<double>(record.substr(pos1, 11));
	pos1 += 11;
	elevation = FromString<double>(record.substr(pos1, 7));
	pos1 += 7;
	local_depth = FromString<double>(record.substr(pos1, 5));
	pos1 += 5;
	azimuth = FromString<double>(record.substr(pos1, 5));
	pos1 += 5;
	dip = FromString<double>(record.substr(pos1, 5));
	pos1 += 5;
	data_format_identifier_code = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	data_record_length = FromString<int>(record.substr(pos1, 2));
	pos1 += 2;
	sample_rate = FromString<double>(record.substr(pos1, 10));
	pos1 += 10;
	max_clock_drift = FromString<double>(record.substr(pos1, 10));
	pos1 += 10;
	number_of_comments = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	flags = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	start_date = SplitString(record, SEED_SEPARATOR, pos1, pos2);
//	if(start_date != "")
//		start_date = FromJulianDay(start_date);
	pos1 = ++pos2;
	end_date = SplitString(record, SEED_SEPARATOR, pos1, pos2);
//	if(end_date != "")
//		end_date = FromJulianDay(end_date);
	pos1 = ++pos2;
	update_flag = record[pos1];
}

/****************************************************************************************************************************
* Function:     EmptyVectors												    *
* Parameters:   none													    *
* Returns:      nothing													    *
* Description:  clear all vectors after the data has been written to inventory						    *
****************************************************************************************************************************/
void ChannelIdentifier::EmptyVectors()
{
	if(!rpz.empty())
		rpz.clear();
	if(!rc.empty())
		rc.clear();
	if(!rl.empty())
		rl.clear();
	if(!gr.empty())
		gr.clear();
	if(!dec.empty())
		dec.clear();
	if(!csg.empty())
		csg.clear();
	if(!cc.empty())
		cc.clear();
	if(!rr.empty())
		rr.clear();
	if(!firr.empty())
		firr.clear();
	if(!rp.empty())
		rp.clear();
}

/****************************************************************************************************************************
* Function:     ResponsePolesZeros											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponsePolesZeros									    *
****************************************************************************************************************************/
ResponsePolesZeros::ResponsePolesZeros(string record)
{
	transfer_function_type  = record[0];
	stage_sequence_number = FromString<int>(record.substr(1, 2));
	signal_in_units = FromString<int>(record.substr(3, 3));
	signal_out_units = FromString<int>(record.substr(6, 3));
	ao_normalization_factor = FromString<double>(record.substr(9, 12));
	normalization_frequency = FromString<double>(record.substr(21, 12));
	number_of_zeros = FromString<int>(record.substr(33, 3));
	int pos1 = 36;
	Zeros cz;
	for(int i=0; i<number_of_zeros; i++)
	{
		cz.real_zero = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cz.imaginary_zero = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cz.real_zero_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cz.imaginary_zero_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		complex_zeros.push_back(cz);
	}
	number_of_poles = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	Poles cp;
	for(int i=0; i<number_of_poles; i++)
	{
		cp.real_pole = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cp.imaginary_pole = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cp.real_pole_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cp.imaginary_pole_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		complex_poles.push_back(cp);
	}
}

/****************************************************************************************************************************
* Function:     GetComplexZeros												    *
* Parameters:   none													    *
* Returns:      string containing the zeros										    *
* Description:  get all the zeros from the ResponsePolesZeros and put them in a formatted string			    *
*		in the case of inventory the format is: ('real_zero', 'imaginary_zero'), etc.				    *
****************************************************************************************************************************/
string ResponsePolesZeros::GetComplexZeros()
{
	string zeros;
	for(int i=0; i<number_of_zeros; i++)
	{
		zeros += '(';
		zeros += ToString<double>(complex_zeros[i].real_zero);
		zeros += ',';
		zeros += ToString<double>(complex_zeros[i].imaginary_zero);
		zeros += ')';
		if(i<number_of_zeros-1)
			zeros += ' ';
	}
	return zeros;
}

/****************************************************************************************************************************
* Function:     GetComplexPoles												    *
* Parameters:   none													    *
* Returns:      string containing the poles										    *
* Description:  get all the poles from the ReponsePolesZeros and put them in a formatted string				    *
*		in the case of inventory the format is: ('real_pole', 'imaginary_pole'), etc.				    *
****************************************************************************************************************************/
string ResponsePolesZeros::GetComplexPoles()
{
	string poles;
	for(int i=0; i<number_of_poles; i++)
	{
		poles += '(';
		poles += ToString<double>(complex_poles[i].real_pole);
		poles += ',';
		poles += ToString<double>(complex_poles[i].imaginary_pole);
		poles += ')';
		if(i<number_of_poles-1)
			poles += ' ';
	}
	return poles;
}

/****************************************************************************************************************************
* Function:     ResponseCoefficients											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponseCoefficients									    *
****************************************************************************************************************************/
ResponseCoefficients::ResponseCoefficients(string record)
{
	response_type  = record[0];
	stage_sequence_number = FromString<int>(record.substr(1, 2));
	signal_in_units = FromString<int>(record.substr(3, 3));
	signal_out_units = FromString<int>(record.substr(6, 3));
	number_of_numerators = FromString<int>(record.substr(9, 4));
	int pos1 = 13;
	Coefficient coeff;
	for(int i=0; i<number_of_numerators; i++)
	{
		coeff.coefficient = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		coeff.error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		numerators.push_back(coeff);
	}
	number_of_denominators = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	for(int i=0; i<number_of_denominators; i++)
	{
		coeff.coefficient = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		coeff.error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		denominators.push_back(coeff);
	}
}

/****************************************************************************************************************************
* Function:     GetNumerators												    *
* Parameters:   none													    *
* Returns:      string with all the numerators										    *
* Description:  get all the numerators from the ResponseCoefficients and put them in a formatted string			    *
****************************************************************************************************************************/
string ResponseCoefficients::GetNumerators()
{
	string num;
	for(int i=0; i<number_of_numerators; i++)
	{
		num += ToString<double>(numerators[i].coefficient);
		num += ' ';
	}
	return num;
}

/****************************************************************************************************************************
* Function:     GetDenominators												    *
* Parameters:   none													    *
* Returns:      string with all the denominators									    *
* Description:  get all the denominators from the ResponseCoefficients and put them in a formatted string		    *
****************************************************************************************************************************/
string ResponseCoefficients::GetDenominators()
{
	string denom;
	for(int i=0; i<number_of_numerators; i++)
	{
		denom += ToString<double>(denominators[i].coefficient);
		denom += ' ';
	}
	return denom;
}

/****************************************************************************************************************************
* Function:     ResponseList												    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponseList										    *
****************************************************************************************************************************/
ResponseList::ResponseList(string record)
{
	stage_sequence_number = FromString<int>(record.substr(0, 2));
	signal_in_units = FromString<int>(record.substr(2, 3));
	signal_out_units = FromString<int>(record.substr(5, 3));
	number_of_responses = FromString<int>(record.substr(8, 4));
	int pos1 = 12;
	ListedResponses lr;
	for(int i=0; i<number_of_responses; i++)
	{
		lr.frequency = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.amplitude= FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.amplitude_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.phase_angle = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		lr.phase_error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		responses_listed.push_back(lr);
	}
}

/****************************************************************************************************************************
* Function:     GenericResponse												    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the GenericResponse										    *
****************************************************************************************************************************/
GenericResponse::GenericResponse(string record)
{
	stage_sequence_number = FromString<int>(record.substr(0, 2));
	signal_in_units = FromString<int>(record.substr(2, 3));
	signal_out_units = FromString<int>(record.substr(5, 3));
	number_of_corners = FromString<int>(record.substr(8, 4));
	int pos1 = 12;
	CornerList cl;
	for(int i=0; i<number_of_corners; i++)
	{
		cl.frequency = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		cl.slope = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		corners_listed.push_back(cl);
	}
}

/****************************************************************************************************************************
* Function:     Decimation												    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the Decimation										    *
****************************************************************************************************************************/
Decimation::Decimation(string record)
{
	stage_sequence_number = FromString<int>(record.substr(0, 2));
	input_sample_rate = FromString<double>(record.substr(2, 10));
	factor = FromString<int>(record.substr(12, 5));
	offset = FromString<int>(record.substr(17, 5));
	estimated_delay = FromString<double>(record.substr(22, 11));
	correction_applied = FromString<double>(record.substr(33, 11));
}

/****************************************************************************************************************************
* Function:     ChannelSensitivityGain											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ChannelSensitivityGain									    *
****************************************************************************************************************************/
ChannelSensitivityGain::ChannelSensitivityGain(string record)
{
	stage_sequence_number = FromString<int>(record.substr(0, 2));
	sensitivity_gain = FromString<double>(record.substr(2, 12));
	frequency = FromString<double>(record.substr(14, 12));
	number_of_history_values = FromString<int>(record.substr(26, 2));
	int pos1 = 28, pos2;
	HistoryValues hv;
	for(int i=0; i<number_of_history_values; i++)
	{
		hv.sensitivity_for_calibration = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		hv.frequency_of_calibration_sensitivity = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		hv.time_of_calibration = SplitString(record, SEED_SEPARATOR, pos1, pos2);
		pos1 = ++pos2;
		history_values.push_back(hv);
	}
}

/****************************************************************************************************************************
* Function:     ResponseReference											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponseReference									    *
****************************************************************************************************************************/
ResponseReference::ResponseReference(string record)
{
	number_of_stages = FromString<int>(record.substr(0,2));
	int pos=2;
	for(int i=0; i < number_of_stages; i++)
	{
		stage_sequence_number.push_back(FromString<int>(record.substr(pos, 2)));
		pos += 2;
	}
	number_of_responses = FromString<int>(record.substr(pos,2));
	pos += 2;
	for(int i=0; i < number_of_responses; i++)
	{
		response_lookup_key.push_back(FromString<int>(record.substr(pos, 4)));
		pos += 4;
	}
}

/****************************************************************************************************************************
* Function:     FIRResponse												    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the FIRResponse										    *
****************************************************************************************************************************/
FIRResponse::FIRResponse(string record)
{
	stage_sequence_number = FromString<int>(record.substr(0, 2));
	int pos1=2, pos2;
	response_name = SplitString(record, SEED_SEPARATOR, pos1, pos2);
	pos1 = ++pos2;
	symmetry_code = record[pos1++];
	signal_in_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	signal_out_units = FromString<int>(record.substr(pos1, 3));
	pos1 += 3;
	number_of_coefficients = FromString<int>(record.substr(pos1, 4));
	pos1 += 4;
	for(int i=0; i < number_of_coefficients; i++)
	{
		coefficients.push_back(FromString<double>(record.substr(pos1, 14)));
		pos1 += 14;
	}
}

string FIRResponse::GetCoefficients()
{
      string num;
      for(int i=0; i<number_of_coefficients; i++)
      {
              num += ToString<double>(coefficients[i]);
              num += ' ';
      }
      return num;
}

/****************************************************************************************************************************
* Function:     ResponsePolynomial											    *
* Parameters:   record	- string with data of this blockette								    *
* Description:  initialize the ResponsePolynomial									    *
****************************************************************************************************************************/
ResponsePolynomial::ResponsePolynomial(string record)
{
	transfer_function_type  = record[0];
	stage_sequence_number = FromString<int>(record.substr(1, 2));
	signal_in_units = FromString<int>(record.substr(3, 3));
	signal_out_units = FromString<int>(record.substr(6, 3));
	polynomial_approximation_type  = record[9];
	valid_frequency_units  = record[10];
	lower_valid_frequency_bound = FromString<double>(record.substr(11, 12));
	upper_valid_frequency_bound = FromString<double>(record.substr(23, 12));
	lower_bound_of_approximation = FromString<double>(record.substr(35, 12));
	upper_bound_of_approximation = FromString<double>(record.substr(47, 12));
	maximum_absolute_error = FromString<double>(record.substr(59, 12));
	number_of_pcoeff = FromString<int>(record.substr(71, 3));
	int pos1 = 74;
	Coefficient pc;
	for(int i=0; i<number_of_pcoeff; i++)
	{
		pc.coefficient = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		pc.error = FromString<double>(record.substr(pos1, 12));
		pos1 += 12;
		polynomial_coefficients.push_back(pc);
	}
}

string ResponsePolynomial::GetPolynomialCoefficients()
{
      string num;
      for(int i=0; i<number_of_pcoeff; i++)
      {
              num += ToString<double>(polynomial_coefficients[i].coefficient);
              num += ' ';
      }
      return num;
}

