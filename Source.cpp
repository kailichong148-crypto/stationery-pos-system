#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <limits>
#include <algorithm>
#include <cctype>
using namespace std;

// ============================================================
//  Constants
// ============================================================
const int    MAX_ITEMS = 10;
const int    MAX_PURCHASE = 10;   // max unique items per transaction
const double PROMO_THRESHOLD = 50.0;
const double PROMO_DISCOUNT = 0.10;
const string PASSWORD = "admin123";
const int    MAX_LOGIN_TRIES = 3;
const int    MAX_NAME_LEN = 40;

// ============================================================
//  Helpers – UI chrome
// ============================================================
void printLine(char c = '-', int w = 52) { cout << string(w, c) << "\n"; }

void printHeader(const string& title) {
    printLine('=');
    int pad = (52 - (int)title.size()) / 2;
    cout << string(pad, ' ') << title << "\n";
    printLine('=');
}

void printSuccess(const string& msg) { cout << "  [OK]  " << msg << "\n"; }
void printError(const string& msg) { cout << " [ERR]  " << msg << "\n"; }
void printInfo(const string& msg) { cout << " [---]  " << msg << "\n"; }

// ============================================================
//  Helpers – safe input
// ============================================================

// Read a non-empty trimmed string (no leading/trailing spaces).
// Returns false if the user just pressed Enter on an empty line.
bool readLine(string& out, const string& prompt) {
    cout << prompt;
    if (!getline(cin, out)) return false;
    // trim
    size_t s = out.find_first_not_of(" \t");
    size_t e = out.find_last_not_of(" \t");
    out = (s == string::npos) ? "" : out.substr(s, e - s + 1);
    return !out.empty();
}

// Read a positive integer in [minVal, maxVal].  Rejects non-numeric input.
bool readInt(int& out, const string& prompt, int minVal, int maxVal) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) return false;
        if (line.empty()) { printError("Please enter a number."); continue; }
        bool isNum = true;
        for (char c : line) if (!isdigit(c)) { isNum = false; break; }
        if (!isNum) { printError("Invalid input – digits only."); continue; }
        int v = stoi(line);
        if (v < minVal || v > maxVal) {
            printError("Value must be between " + to_string(minVal) +
                " and " + to_string(maxVal) + ".");
            continue;
        }
        out = v;
        return true;
    }
}

// Read a non-negative double.
bool readDouble(double& out, const string& prompt, double minVal) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) return false;
        if (line.empty()) { printError("Please enter a number."); continue; }
        try {
            size_t pos;
            double v = stod(line, &pos);
            if (pos != line.size()) throw invalid_argument("");
            if (v < minVal) {
                printError("Value must be >= " + to_string(minVal));
                continue;
            }
            out = v;
            return true;
        }
        catch (...) {
            printError("Invalid number – please try again.");
        }
    }
}

// Read a single-character menu choice from a given valid set.
char readChoice(const string& validChars, const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) return 0;
        if (line.size() == 1 &&
            validChars.find(line[0]) != string::npos)
            return line[0];
        printError("Please enter one of: " + validChars);
    }
}

// Yes / No confirmation
bool confirm(const string& question) {
    char c = readChoice("YyNn", question + " (Y/N): ");
    return (c == 'Y' || c == 'y');
}

// ============================================================
//  Item structure
// ============================================================
struct Item {
    string name = "";
    double price = 0.0;
    int    quantity = 0;
};

// ============================================================
//  Function declarations
// ============================================================
bool login();
void displayItems(const Item items[], int size);
void processPurchase(Item items[], int size);
void saveReceipt(const string& customerName,
    const Item purchasedItems[], int purchaseCount,
    double total, double discount,
    const string& paymentMethod);
void showReceipts();
void generateSalesReport();
void updateInventory(Item items[], int& currentSize);
void processReturn(Item items[], int size);

// ============================================================
//  main
// ============================================================
int main() {
    if (!login()) {
        printError("Access denied. Exiting.");
        return 1;
    }

    Item items[MAX_ITEMS] = {
        {"Notebook", 2.50, 50},
        {"Pen",      1.00, 100},
        {"Pencil",   0.50, 100},
        {"Eraser",   0.75, 80},
        {"Ruler",    1.20, 60}
    };
    int currentSize = 5;

    printHeader("STATIONERY STORE POS SYSTEM");

    char choice;
    do {
        cout << "\n";
        printLine();
        cout << "  MAIN MENU\n";
        printLine();
        cout << "  1. View Items\n"
            << "  2. Purchase Items\n"
            << "  3. Show Latest Receipt\n"
            << "  4. Generate Sales Report\n"
            << "  5. Update Inventory\n"
            << "  6. Process Return\n"
            << "  7. Exit\n";
        printLine();
        choice = readChoice("1234567", "  Enter choice: ");

        switch (choice) {
        case '1': displayItems(items, currentSize);        break;
        case '2': processPurchase(items, currentSize);     break;
        case '3': showReceipts();                          break;
        case '4': generateSalesReport();                   break;
        case '5': updateInventory(items, currentSize);     break;
        case '6': processReturn(items, currentSize);       break;
        case '7':
            if (confirm("Exit the POS system?"))
                cout << "\n  Thank you. Goodbye!\n\n";
            else
                choice = '0'; // keep looping
            break;
        }
    } while (choice != '7');

    return 0;
}

// ============================================================
//  Login  (max 3 attempts)
// ============================================================
bool login() {
    printHeader("SYSTEM LOGIN");
    for (int attempt = 1; attempt <= MAX_LOGIN_TRIES; ++attempt) {
        cout << "  Attempt " << attempt << " of " << MAX_LOGIN_TRIES << "\n";
        string pw;
        if (!readLine(pw, "  Password: ")) continue;
        if (pw == PASSWORD) {
            printSuccess("Login successful.");
            return true;
        }
        printError("Incorrect password.");
    }
    return false;
}

// ============================================================
//  Display items
// ============================================================
void displayItems(const Item items[], int size) {
    cout << "\n";
    printLine();
    cout << left
        << setw(5) << " No."
        << setw(18) << " Name"
        << setw(12) << " Price(RM)"
        << " Stock\n";
    printLine();
    for (int i = 0; i < size; ++i) {
        cout << " " << setw(4) << i + 1
            << " " << setw(17) << items[i].name
            << " " << setw(11) << fixed << setprecision(2) << items[i].price
            << " " << items[i].quantity;
        if (items[i].quantity == 0)      cout << "  [OUT OF STOCK]";
        else if (items[i].quantity <= 5) cout << "  [LOW STOCK]";
        cout << "\n";
    }
    printLine();
}

// ============================================================
//  Process purchase
// ============================================================
void processPurchase(Item items[], int size) {
    printHeader("NEW PURCHASE");

    string customerName;
    while (!readLine(customerName, "  Customer name: ")) {
        printError("Name cannot be empty.");
    }
    if ((int)customerName.size() > MAX_NAME_LEN) {
        customerName = customerName.substr(0, MAX_NAME_LEN);
        printInfo("Name truncated to " + to_string(MAX_NAME_LEN) + " characters.");
    }

    Item   purchasedItems[MAX_PURCHASE];
    int    purchasedIndex[MAX_PURCHASE]; // tracks which item[] slot was bought
    int    purchaseCount = 0;
    double total = 0.0;

    cout << "\n  Add items to cart (enter 0 to finish).\n";

    while (purchaseCount < MAX_PURCHASE) {
        displayItems(items, size);

        int itemIndex;
        if (!readInt(itemIndex, "\n  Item number (0 = finish): ", 0, size))
            break;
        if (itemIndex == 0) break;

        // Check stock
        if (items[itemIndex - 1].quantity == 0) {
            printError("That item is out of stock.");
            continue;
        }

        int qty;
        if (!readInt(qty, "  Quantity: ", 1, items[itemIndex - 1].quantity)) break;

        // Check for duplicate in cart and merge
        bool merged = false;
        for (int j = 0; j < purchaseCount; ++j) {
            if (purchasedIndex[j] == itemIndex - 1) {
                int combined = purchasedItems[j].quantity + qty;
                if (combined > items[itemIndex - 1].quantity) {
                    printError("Total quantity (" + to_string(combined) +
                        ") exceeds available stock (" +
                        to_string(items[itemIndex - 1].quantity) + ").");
                }
                else {
                    purchasedItems[j].quantity = combined;
                    total += qty * items[itemIndex - 1].price;
                    printSuccess(items[itemIndex - 1].name + " updated in cart.");
                }
                merged = true;
                break;
            }
        }

        if (!merged) {
            items[itemIndex - 1].quantity -= qty;
            purchasedItems[purchaseCount] = items[itemIndex - 1];
            purchasedItems[purchaseCount].quantity = qty;
            purchasedIndex[purchaseCount] = itemIndex - 1;
            total += qty * items[itemIndex - 1].price;
            purchaseCount++;
            printSuccess(items[itemIndex - 1].name + " added to cart.");
        }
    }

    if (purchaseCount == MAX_PURCHASE)
        printInfo("Cart limit reached (" + to_string(MAX_PURCHASE) + " items).");

    if (purchaseCount == 0) {
        printInfo("No items purchased.");
        return;
    }

    // Show cart summary
    cout << "\n";
    printLine();
    cout << "  CART SUMMARY\n";
    printLine();
    cout << left << setw(18) << "  Item" << setw(10) << "Price" << setw(8) << "Qty" << "Subtotal\n";
    printLine();
    for (int i = 0; i < purchaseCount; ++i) {
        double sub = purchasedItems[i].price * purchasedItems[i].quantity;
        cout << "  " << setw(16) << purchasedItems[i].name
            << "  " << setw(8) << fixed << setprecision(2) << purchasedItems[i].price
            << "  " << setw(6) << purchasedItems[i].quantity
            << "  " << fixed << setprecision(2) << sub << "\n";
    }
    printLine();

    double discount = (total > PROMO_THRESHOLD) ? total * PROMO_DISCOUNT : 0.0;
    double finalTotal = total - discount;

    if (discount > 0)
        printSuccess("10% promo discount applied! You save RM" +
            to_string(discount).substr(0, to_string(discount).find('.') + 3));

    cout << "  Subtotal : RM" << fixed << setprecision(2) << total << "\n";
    cout << "  Discount : RM" << fixed << setprecision(2) << discount << "\n";
    cout << "  TOTAL    : RM" << fixed << setprecision(2) << finalTotal << "\n";
    printLine();

    if (!confirm("  Confirm purchase?")) {
        // Restore stock
        for (int i = 0; i < purchaseCount; ++i)
            items[purchasedIndex[i]].quantity += purchasedItems[i].quantity;
        printInfo("Purchase cancelled. Stock restored.");
        return;
    }

    // Payment method
    cout << "\n  Payment Method:\n"
        << "  1. Cash\n"
        << "  2. Touch 'n Go (TnG)\n"
        << "  3. Card\n";
    char pm = readChoice("123", "  Select: ");
    string paymentMethodStr;
    switch (pm) {
    case '1': paymentMethodStr = "Cash";              break;
    case '2': paymentMethodStr = "Touch 'n Go (TnG)"; break;
    case '3': paymentMethodStr = "Card";              break;
    }

    // Cash: calculate change
    if (pm == '1') {
        double paid = 0.0;
        while (true) {
            if (!readDouble(paid, "  Amount tendered (RM): ", 0.0)) return;
            if (paid < finalTotal) {
                printError("Amount tendered is less than total (RM" +
                    to_string(finalTotal).substr(0, to_string(finalTotal).find('.') + 3) + ").");
            }
            else break;
        }
        cout << "  Change   : RM" << fixed << setprecision(2) << paid - finalTotal << "\n";
    }

    saveReceipt(customerName, purchasedItems, purchaseCount,
        total, discount, paymentMethodStr);
    printSuccess("Payment accepted via " + paymentMethodStr + ". Receipt saved.");
}

// ============================================================
//  Save receipt + sales log
// ============================================================
void saveReceipt(const string& customerName,
    const Item purchasedItems[], int purchaseCount,
    double total, double discount,
    const string& paymentMethod) {
    ofstream receiptFile("receipts.txt", ios::app);
    ofstream salesFile("sales_report.txt", ios::app);

    if (!receiptFile || !salesFile) {
        printError("Could not open file(s) for writing.");
        return;
    }

    receiptFile << string(52, '-') << "\n"
        << "Customer : " << customerName << "\n"
        << "Payment  : " << paymentMethod << "\n"
        << string(52, '-') << "\n"
        << left << setw(18) << "Item"
        << setw(10) << "Price"
        << setw(8) << "Qty"
        << "Subtotal\n"
        << string(52, '-') << "\n";

    for (int i = 0; i < purchaseCount; ++i) {
        double sub = purchasedItems[i].price * purchasedItems[i].quantity;
        receiptFile << setw(18) << purchasedItems[i].name
            << setw(10) << fixed << setprecision(2) << purchasedItems[i].price
            << setw(8) << purchasedItems[i].quantity
            << fixed << setprecision(2) << sub << "\n";

        salesFile << customerName << "\t"
            << purchasedItems[i].name << "\t"
            << purchasedItems[i].quantity << "\t"
            << purchasedItems[i].price << "\t"
            << sub << "\n";
    }

    receiptFile << string(52, '-') << "\n"
        << "Subtotal : RM" << fixed << setprecision(2) << total << "\n"
        << "Discount : RM" << fixed << setprecision(2) << discount << "\n"
        << "TOTAL    : RM" << fixed << setprecision(2) << total - discount << "\n\n";
}

// ============================================================
//  Show latest receipt
// ============================================================
void showReceipts() {
    ifstream f("receipts.txt", ios::ate);
    if (!f) { printError("No receipts file found."); return; }
    if (f.tellg() == 0) { printInfo("No receipts on record."); return; }
    f.seekg(0);

    string line, current, last;
    while (getline(f, line)) {
        if (line.empty()) {
            if (!current.empty()) { last = current; current.clear(); }
        }
        else {
            current += line + "\n";
        }
    }
    if (!current.empty()) last = current;

    printHeader("LATEST RECEIPT");
    cout << last << "\n";
}

// ============================================================
//  Sales report
// ============================================================
void generateSalesReport() {
    ifstream f("sales_report.txt");
    if (!f) { printError("No sales report file found."); return; }

    string customer, item;
    int    qty;
    double price, sub, grand = 0.0;
    int    lines = 0;

    printHeader("SALES REPORT");
    cout << left
        << setw(18) << "Customer"
        << setw(15) << "Item"
        << setw(8) << "Qty"
        << setw(10) << "Price(RM)"
        << "Subtotal\n";
    printLine();

    while (f >> customer >> item >> qty >> price >> sub) {
        cout << setw(18) << customer
            << setw(15) << item
            << setw(8) << qty
            << setw(10) << fixed << setprecision(2) << price
            << fixed << setprecision(2) << sub << "\n";
        grand += sub;
        ++lines;
    }

    if (lines == 0) { printInfo("No sales data yet."); return; }
    printLine();
    cout << "  TOTAL SALES: RM" << fixed << setprecision(2) << grand << "\n";
    printLine();
}

// ============================================================
//  Update inventory
// ============================================================
void updateInventory(Item items[], int& currentSize) {
    printHeader("UPDATE INVENTORY");
    cout << "  1. Restock existing item\n"
        << "  2. Add new item\n";
    char c = readChoice("12", "  Choice: ");

    if (c == '1') {
        displayItems(items, currentSize);
        int idx;
        if (!readInt(idx, "  Item number to restock: ", 1, currentSize)) return;
        int qty;
        if (!readInt(qty, "  Quantity to add: ", 1, 9999)) return;
        items[idx - 1].quantity += qty;
        printSuccess(items[idx - 1].name + " stock updated to " +
            to_string(items[idx - 1].quantity) + ".");
    }
    else {
        if (currentSize >= MAX_ITEMS) {
            printError("Inventory full (max " + to_string(MAX_ITEMS) + " items).");
            return;
        }

        string name;
        while (true) {
            if (!readLine(name, "  New item name: "))
            {
                printError("Name cannot be empty."); continue;
            }
            if ((int)name.size() > MAX_NAME_LEN)
            {
                printError("Name too long (max " + to_string(MAX_NAME_LEN) + " chars)."); continue;
            }
            // Check duplicate (case-insensitive)
            bool dup = false;
            for (int i = 0; i < currentSize; ++i) {
                string a = items[i].name, b = name;
                transform(a.begin(), a.end(), a.begin(), ::tolower);
                transform(b.begin(), b.end(), b.begin(), ::tolower);
                if (a == b) { dup = true; break; }
            }
            if (dup) { printError("An item with that name already exists."); continue; }
            break;
        }

        double price;
        if (!readDouble(price, "  Price (RM): ", 0.01)) return;

        int qty;
        if (!readInt(qty, "  Initial quantity: ", 0, 9999)) return;

        items[currentSize] = { name, price, qty };
        currentSize++;
        printSuccess("\"" + name + "\" added to inventory.");
    }

    // Persist to file
    ofstream inv("inventory.txt");
    if (!inv) { printError("Could not save inventory file."); return; }
    for (int i = 0; i < currentSize; ++i)
        inv << items[i].name << " " << items[i].price << " " << items[i].quantity << "\n";
    printSuccess("Inventory saved to file.");
}

// ============================================================
//  Process return
// ============================================================
void processReturn(Item items[], int size) {
    printHeader("PROCESS RETURN");

    string customerName;
    while (!readLine(customerName, "  Customer name: "))
        printError("Name cannot be empty.");

    // Select item from list (safer than typing a name)
    displayItems(items, size);
    int idx;
    if (!readInt(idx, "  Item number to return (0 = cancel): ", 0, size)) return;
    if (idx == 0) { printInfo("Return cancelled."); return; }

    int qty;
    if (!readInt(qty, "  Quantity to return: ", 1, 9999)) return;

    // Warn if returning more than a reasonable amount
    double refund = qty * items[idx - 1].price;

    cout << "\n  Return summary:\n"
        << "  Item     : " << items[idx - 1].name << "\n"
        << "  Qty      : " << qty << "\n"
        << "  Refund   : RM" << fixed << setprecision(2) << refund << "\n";

    if (!confirm("  Confirm return?")) {
        printInfo("Return cancelled.");
        return;
    }

    items[idx - 1].quantity += qty;

    ofstream f("receipts.txt", ios::app);
    if (f) {
        f << string(52, '-') << "\n"
            << "Customer : " << customerName << "\n"
            << "Type     : RETURN\n"
            << string(52, '-') << "\n"
            << left << setw(18) << "Item"
            << setw(10) << "Price" << setw(8) << "Qty" << "Refund\n"
            << string(52, '-') << "\n"
            << setw(18) << items[idx - 1].name
            << setw(10) << fixed << setprecision(2) << items[idx - 1].price
            << setw(8) << qty
            << fixed << setprecision(2) << refund << "\n"
            << string(52, '-') << "\n"
            << "REFUND   : RM" << fixed << setprecision(2) << refund << "\n\n";
    }
    else {
        printError("Could not write return receipt.");
    }

    printSuccess("Return processed. RM" +
        to_string(refund).substr(0, to_string(refund).find('.') + 3) +
        " refunded to " + customerName + ".");
}