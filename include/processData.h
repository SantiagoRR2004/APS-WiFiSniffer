#define SERIAL_DEBUG false
bool is_kg;
bool is_estable;
String udString[] = {"_", "kg", "kg", "gr"};
String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
boolean isValidNumber(String str)
{
    for (byte i = 0; i < str.length(); i++)
    {
        if (isDigit(str.charAt(i)))
            return true;
    }
    return false;
}
bool processDataFrame(String data, int modelo, bool convertToG, float &peso, bool &estable)
{
    peso = 0;
    switch (modelo)
    {
    case 0: // rawdata
        /* code */
        break;
    case 1: // bizerba
        // Serial.println(data);
        is_kg = data.indexOf("kg");
        is_estable = data.indexOf("+!");
        data = data.substring(3, 11);
        data.replace(",", ".");
        // Serial.println(data);
        if (!isValidNumber(data))
            return false;
        peso = data.toFloat();
        if (is_kg && convertToG)
            peso = 1000 * peso;
        // Serial.println(data);
        break;
    case 2: // Br16
        data = getValue(data, ',', 2);
        data = data.substring(0, data.indexOf("kg"));
        if (!isValidNumber(data))
            return false;
        peso = data.toFloat();
        if (is_kg && convertToG)
            peso = 1000 * peso;
        break;
    case 3: // Varpe
        if (!isValidNumber(data))
        {
            return false;
        }
        peso = data.toFloat();
        break;
    default:
        break;
    }
    return true;
}

bool readDataSerial(HardwareSerial &HardwSerial, int modelo, bool convertToG, float &peso, bool &estable)
{
    switch (modelo)
    {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
        String dataSerial = Serial1.readStringUntil('\r');
        if (SERIAL_DEBUG) Serial.println(dataSerial);
        Serial1.readStringUntil('\n');
        if (SERIAL_DEBUG) Serial.println(dataSerial);
        return processDataFrame(dataSerial, modelo, convertToG, peso, estable);
        break;
    }
}