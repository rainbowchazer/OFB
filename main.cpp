#include <iostream>
#include <string>
#include <math.h>
#include <vector>
#include <bitset>
#include <conio.h>
#include <windows.h>

#include "table.h"
#include "key.h"

using namespace std;

#define TOTAL_ROUND 16

// ��������������� �������
int XOR(int a, int b) {
    if (a == b) {
        return 0;
    }
    return 1;
}

vector<int> blockXOR(vector<int> a, vector<int> b) {
    vector<int> res;
    for (int x = 0; x < a.size(); x++) {
        res.push_back(XOR(a.at(x), b.at(x)));
    }
    return res;
}


vector<int> numToBit(int n, int size, bool pad) {
    vector<int> bits;

    while (n > 0) {
        bits.insert(bits.begin(), n % 2);
        n /= 2;
    }

    int pads = size - bits.size();
    if (pad && pads > 0) {
        for (int x = 0; x < pads; x++) {
            bits.insert(bits.begin(), 0);
        }
    }

    return bits;
}


vector<int> charToBit(char c, int size, bool pad, bool cap) {
    int n = int(c);

    if (n < 0) {
        n = 256 + n;
    }

    vector<int> bits;

    while (n > 0) {
        bits.insert(bits.begin(), n % 2);
        n /= 2;
    }

    int pads = size - bits.size();
    if (pad && pads > 0) {
        for (int x = 0; x < pads; x++) {
            bits.insert(bits.begin(), 0);
        }
    }
    else if (cap) {
        for (int x = pads; x < 0; x++) {
            bits.erase(bits.begin());
        }
    }

    return bits;
}


vector<int> strToBit(string str, int size, bool pad, bool cap) {
    vector<int> bits;
    for (char c : str) {
        vector<int> tmpBits;
        for (int bit : charToBit(c, 8, true, false)) {
            bits.push_back(bit);
        }
    }

    int pads = size - bits.size();
    if (pad && pads > 0) {
        for (int x = 0; x < pads; x++) {
            bits.push_back(0);
        }
    }
    else if (cap) {
        for (int x = pads; x < 0; x++) {
            bits.erase(bits.begin());
        }
    }

    return bits;
}

vector<vector<int>> strToBit64Blocks(string str, bool pad, bool cap) {
    vector<vector<int>> blocks;

    int n = str.size();
    int totalBlocks = ceil(float(n) / 8);

    int s;
    for (int x = 0; x < totalBlocks; x++) {
        s = 8;
        if (x == totalBlocks - 1 && (n % 8) != 0) {
            s = n % 8;
        }

        blocks.push_back(strToBit(str.substr(x * 8, s), 64, pad, cap));
    }

    return blocks;
}


string bit64ToStr(vector<int> bits) {
    string res = "";

    vector<vector<int>> grps;
    for (int x = 0; x < 8; x++) {
        grps.push_back(vector<int>(bits.begin() + (x * 8), bits.begin() + ((x * 8) + 8)));
    }

    int d = 0;
    int p = 7;
    for (vector<int> x : grps) {
        for (int y : x) {
            d += pow(2, p--) * y;
        }
        /* d %= 128; */
        res += (char)d;
        d = 0;
        p = 7;
    }

    return res;
}

string bit64BlocksToStr(vector<vector<int>> blocks) {
    string res = "";

    for (vector<int> block : blocks) {
        res += bit64ToStr(block);
    }

    return res;
}


vector<vector<int>> desKeygen(vector<int> key) {
    vector<vector<int>> result;
    vector<int> pc1, pc2;

    // ������������ � �������� PC1
    for (int x : PC1) {
        pc1.push_back(key.at(x - 1));
    }

    // ���������� �� ��� �����, (c, d)
    vector<vector<int>> c;
    vector<vector<int>> d;

    c.push_back(vector<int>(pc1.begin(), pc1.begin() + 28));
    d.push_back(vector<int>(pc1.begin() + 28, pc1.begin() + 56));

    // ������� ��������
    vector<int> shiftbits;
    vector<int> otherbits;
    for (int count : ITERATION_SHIFT) {
        shiftbits = vector<int>(c.back().begin(), c.back().begin() + count);
        otherbits = vector<int>(c.back().begin() + count, c.back().end());

        otherbits.insert(otherbits.end(), shiftbits.begin(), shiftbits.end());
        c.push_back(otherbits);

        shiftbits = vector<int>(d.back().begin(), d.back().begin() + count);
        otherbits = vector<int>(d.back().begin() + count, d.back().end());

        otherbits.insert(otherbits.end(), shiftbits.begin(), shiftbits.end());
        d.push_back(otherbits);
    }

    // �������� n=0
    c.erase(c.begin());
    d.erase(d.begin());

    vector<int> concat;
    for (int n = 0; n < 16; n++) {
        concat.insert(concat.begin(), c[n].begin(), c[n].end());
        concat.insert(concat.end(), d[n].begin(), d[n].end());

        // ������������ � �������� PC2
        for (int x : PC2) {
            pc2.push_back(concat.at(x - 1));
        }
        result.push_back(pc2);
        pc2.clear();

        concat.clear();
    }

    return result;
}

vector<int> desRun(vector<vector<int>> subkeys, vector<int> data) {
    vector<int> ip;

    // ������������ � �������� IP, [64]
    for (int x : IP) {
        ip.push_back(data.at(x - 1));
    }

    // ���������� �� ��� �����, left[32] � right[32]
    vector<vector<int>> left;
    vector<vector<int>> right;

    left.push_back(vector<int>(ip.begin(), ip.begin() + 32));
    right.push_back(vector<int>(ip.begin() + 32, ip.begin() + 64));

    vector<vector<int>> grps;
    vector<int> f, fxk, sbox, sbCur, fRes, lxf;
    for (int n = 0; n < 16; n++) {
        // right[n] = left[n - 1] XOR f(right[n - 1], subkey[n])
        // f(right[n - 1], subkey[n])

        // ���������� right[n - 1] �� [32] � [48] � �������������� E ���-������
        for (int x : EBITSELECT) {
            f.push_back(right.back().at(x - 1));
        }

        // XOR � ���������, fxk
        for (int x = 0; x < 48; x++) {
            fxk.push_back(XOR(f.at(x), subkeys.at(n).at(x)));
        }

        // ���������� �� 8 ����� �� 6 ���
        for (int x = 0; x < 8; x++) {
            grps.push_back(vector<int>(fxk.begin() + (x * 6), fxk.begin() + ((x * 6) + 6)));
        }

        // S-����, � [32]
        int i, j;
        int sbIndex = 0;
        for (vector<int> grp : grps) {
            // ����������� i � j

            // i - ������ � ��������� ��� � ���������� ����, �� 0 �� 3
            i = (grp.at(0) * 2) + (grp.at(5) * 1);

            // j - ������� 4 ���� � ���������� ����, �� 0 �� 15
            j = 0;

            for (int y = 1; y <= 4; y++) {
                j += grp.at(y) * pow(2, 4 - y);
            }

            // ��������� ����������� ����� �� S-�����[n] � �������������� i ��� ������, j ��� �������
            int d = SBOX[sbIndex][i * 16 + j];

            // �������������� � �������� �������������
            sbCur = numToBit(d, 4, true);

            // ����������� � ��������� S-�����
            sbox.insert(sbox.end(), sbCur.begin(), sbCur.end());

            sbIndex++;
        }

        // ������������� ��������� � f(), ������������ � �������� P
        for (int x : PBOX) {
            fRes.push_back(sbox.at(x - 1));
        }

        // right[n] = left[n - 1] XOR f(right[n - 1], subkey[n])
        // XOR � left[n - 1] ��� ��������� right[n]
        for (int x = 0; x < 32; x++) {
            lxf.push_back(XOR(left.back().at(x), fRes.at(x)));
        }

        // left[n] = right[n - 1]
        left.push_back(right.back());
        // right[n] = ^ (lxf)
        right.push_back(lxf);

        grps.clear();
        f.clear();
        fxk.clear();
        sbox.clear();
        sbCur.clear();
        fRes.clear();
        lxf.clear();
    }
    vector<int> rl;

    // �� ��������� ������� [16]
    // �������� left � right ��� right � left
    // �����������
    rl.insert(rl.end(), right.back().begin(), right.back().end());
    rl.insert(rl.end(), left.back().begin(), left.back().end());

    // ������������ � �������� �������� IP, IIP
    vector<int> result;
    for (int x : IIP) {
        result.push_back(rl.at(x - 1));
    }

    return result;
}

vector<int> desEncrypt(vector<int> key, vector<int> data) {
    vector<vector<int>> subkeys = desKeygen(key);

    return desRun(subkeys, data);
}


// OFB � �������������� 2D �������
vector<vector<int>> ofb(vector<vector<int>> data, vector<int> iv, vector<int> key) {
    vector<vector<int>> o; // �����
    vector<vector<int>> c; // ���������

    // p, �������� �����
    for (vector<int> p : data) {
        o.push_back(desEncrypt(key, iv));

        // XOR � �������� �������
        c.push_back(blockXOR(p, o.back()));

        // ��������� �����
        iv = o.back();
    }

    return c;
}


std::string generateRandomString(int length) {
    const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const int charsetLength = charset.length();
    std::string randomString;

    // ������ ��������� ����� ��� ���������
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (int i = 0; i < length; ++i) {
        randomString += charset[std::rand() % charsetLength];
    }

    return randomString;
}

std::string generateRandomVector(int length) {
    const std::string charset = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const int charsetLength = charset.length();
    std::string randomString;

    // ������ ��������� ����� ��� ���������
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (int i = 0; i < length; ++i) {
        randomString += charset[std::rand() % charsetLength];
    }

    return randomString;
}


int main()
{
    setlocale(LC_ALL, "Russian");
    SetConsoleCP(1251);
    srand(time(nullptr));
    int offset = 65;
    char option;
    std::cout << "\t\t������������ ������ �4" << std::endl;
    for (int i = 0; i < offset; i++) std::cout << "-";
    std::cout << std::endl;
    std::cout << "���������� �������� ���������� DES � ������ OFB ��� ���������� ���������." << std::endl;
    do
    {
        std::cout << "1. ����������� ������� ������������� ��������� (������� '1')." << std::endl
            << "2. ����������� ���-�� ���� (������� '2')." << std::endl
            << "3. ����� �� ��������� (������� '0')" << std::endl
            << "����: ";
        std::cin >> option;
        for (int i = 0; i < offset; i++) std::cout << "-";
        std::cout << std::endl;
        switch (option)
        {
        case '1':
        {
            // ������������� �����, ������� � ������ ��� ����������
            string key = generateRandomString(8); 
            string iv = generateRandomVector(8);  
            cout << "��������������� ����: " << key << std::endl;
            cout << "��������������� ��������� ������: " << iv << std::endl;
            string text = "Bochkareva Anastasia Alekseevna";

            // ����������
            vector<vector<int>> textblocks = strToBit64Blocks(text, true, true);
            vector<int> k = strToBit(key, 64, true, true);
            vector<int> i = strToBit(iv, 64, true, true);
            vector<vector<int>> cipherblocks = ofb(textblocks, i, k);

            cout << "���������, ������� �� ������: " << text << endl;
            cout << "��� ��� � ������������� �����: "  << endl;
            for (vector<int> b : cipherblocks) {
                for (int x : b) {
                    cout << x;
                }
                cout << " ";
            }
            cout << "\n\n";

            // �������������
            vector<vector<int>> decrypted_blocks = ofb(cipherblocks, i, k);
            string decrypted_text = bit64BlocksToStr(decrypted_blocks);

            // ����� ��������������� ������
            cout << "�������������� ���������:\n";
            cout << decrypted_text << endl;
            std::cout << std::endl;

            string text1 = "Nizhegorodskij gosudarstvennyj tekhnicheskij universitet";
            vector<vector<int>> textblocks1 = strToBit64Blocks(text1, true, true);
            vector<int> k1 = strToBit(key, 64, true, true);
            vector<int> i1 = strToBit(iv, 64, true, true);
            vector<vector<int>> cipherblocks1 = ofb(textblocks1, i1, k1);

            cout << "���������, ������� �� ������: " << text1 << endl;
            cout << "��� ��� � ������������� �����: " << endl;
            for (vector<int> b : cipherblocks1) {
                for (int x : b) {
                    cout << x;
                }
                cout << " ";
            }
            cout << "\n\n";

            // �������������
            vector<vector<int>> decrypted_blocks1 = ofb(cipherblocks1, i1, k1);
            string decrypted_text1 = bit64BlocksToStr(decrypted_blocks1);

            // ����� ��������������� ������
            cout << "�������������� ���������:\n";
            cout << decrypted_text1 << endl;
            std::cout << std::endl;
            break;
        }
        case '2':
        {
            // ������������� �����, ������� � ������ ��� ����������
            string key = generateRandomString(8); 
            string iv = generateRandomVector(8);  
            cout << "��������������� ����: " << key << std::endl;
            cout << "��������������� ��������� ������: " << iv << std::endl;
            string text = "";
            cout << "������� ���������: ";
            std::cin.ignore();
            std::getline(std::cin, text);

            // ����������
            vector<vector<int>> textblocks = strToBit64Blocks(text, true, true);
            vector<int> k = strToBit(key, 64, true, true);
            vector<int> i = strToBit(iv, 64, true, true);
            vector<vector<int>> cipherblocks = ofb(textblocks, i, k);

            cout << "���������, ������� �� ������: " << text << endl;
            cout << "��� ��� � ������������� �����: "  << endl;
            for (vector<int> b : cipherblocks) {
                for (int x : b) {
                    cout << x;
                }
                cout << " ";
            }
            cout << "\n\n";

            // �������������
            vector<vector<int>> decrypted_blocks = ofb(cipherblocks, i, k);
            string decrypted_text = bit64BlocksToStr(decrypted_blocks);

            // ����� ��������������� ������
            cout << "�������������� ���������:\n";
            cout << decrypted_text << endl;
            std::cout << std::endl;
            system("pause");
            std::cout << std::endl;
            break;
        }
        case '0':
        {
            system("pause");
            break;
        }
        default:
            std::cout << "������������ ����, ���������� ��� ���!" << std::endl;
            std::cout << std::endl;
            system("pause");
            std::cout << std::endl;
            break;
        }
    } while (option != '0');
    return 0;
}