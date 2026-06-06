#include "tests.h"

void setup_large_e_commerce_delivery_mcd(void) {
    // =========================================================================
    // 1. ENTITY INITIALIZATION
    // Spaced out for a spacious terminal grid layout.
    // Entity dimension constraints: Width = 18, Height = 5
    // Spacing between columns: ~40-45 units (leaves ~22-27 units for lines/relationships)
    // Spacing between rows: ~22 units (leaves ~17 units vertically)
    // =========================================================================

    // Row 1 (Top Level: Shopping & Processing)
    Entity *customer = createEntity("Customer", 5, 5);
    Entity *shopping_cart = createEntity("ShoppingCart", 50, 5);
    Entity *order = createEntity("Order", 95, 5);
    Entity *product = createEntity("Product", 140, 5);

    // Row 2 (Bottom Level: Supply Chain & Logistics)
    Entity *delivery = createEntity("Delivery", 5, 27);
    Entity *driver = createEntity("Driver", 50, 27);
    Entity *warehouse = createEntity("Warehouse", 95, 27);
    Entity *supplier = createEntity("Supplier", 140, 27);

    // =========================================================================
    // 2. PROPERTY CONFIGURATIONS
    // Appending metadata attributes within limits (MAX_PROPERTIES = 20)
    // Max length bounds: Name = 15 characters, Type = 6 characters
    // =========================================================================

    // Customer Properties
    addProperty(customer, "customer_id", "int");
    addProperty(customer, "first_name", "str");
    addProperty(customer, "last_name", "str");
    addProperty(customer, "email", "str");
    addProperty(customer, "phone", "str");

    // Shopping Cart Properties
    addProperty(shopping_cart, "cart_id", "int");
    addProperty(shopping_cart, "date_created", "date");
    addProperty(shopping_cart, "is_active", "bool");

    // Order Properties
    addProperty(order, "order_id", "int");
    addProperty(order, "order_date", "date");
    addProperty(order, "total_amount", "float");
    addProperty(order, "pay_status", "str");

    // Product Properties
    addProperty(product, "product_id", "int");
    addProperty(product, "sku", "str");
    addProperty(product, "name", "str");
    addProperty(product, "price", "float");
    addProperty(product, "weight", "float");

    // Delivery Properties
    addProperty(delivery, "delivery_id", "int");
    addProperty(delivery, "track_num", "str");
    addProperty(delivery, "est_arrival", "date");
    addProperty(delivery, "status", "str");

    // Driver Properties
    addProperty(driver, "driver_id", "int");
    addProperty(driver, "license_num", "str");
    addProperty(driver, "phone_num", "str");
    addProperty(driver, "is_active", "bool");

    // Warehouse Properties
    addProperty(warehouse, "warehouse_id", "int");
    addProperty(warehouse, "loc_code", "str");
    addProperty(warehouse, "capacity", "int");

    // Supplier Properties
    addProperty(supplier, "supplier_id", "int");
    addProperty(supplier, "company_name", "str");
    addProperty(supplier, "contact_name", "str");
    addProperty(supplier, "city", "str");

    // =========================================================================
    // 3. RELATIONSHIPS & CARDINALITIES
    // Placed systematically at midpoint vectors between interconnected boxes.
    // Relationship dimensions constraints: Width = 10, Height = 5
    // =========================================================================

    // Customer <-> ShoppingCart (Horizontal path on Row 1)
    // Midpoint X calculation: 5 + 18 + ((50 - (5 + 18)) / 2) - (10 / 2) = 23 + 13 - 5 = 31
    Relationship *r_owns = addRelationship(31, 5, customer, shopping_cart, "Owns");
    addCardinalityAPI("1,1,0,1", r_owns);

    // ShoppingCart <-> Order (Horizontal path on Row 1)
    // Midpoint X calculation: 50 + 18 + ((95 - (50 + 18)) / 2) - (10 / 2) = 68 + 13 - 5 = 76
    Relationship *r_checkout = addRelationship(76, 5, shopping_cart, order, "Checkout");
    addCardinalityAPI("0,1,1,1", r_checkout);

    // Order <-> Product (Horizontal path on Row 1)
    // Midpoint X calculation: 95 + 18 + ((140 - (95 + 18)) / 2) - (10 / 2) = 113 + 13 - 5 = 121
    Relationship *r_order_line = addRelationship(121, 5, order, product, "Order_Line");
    addPropertyRelationship(r_order_line, "quantity", "int");
    addPropertyRelationship(r_order_line, "unit_price", "float");
    addCardinalityAPI("1,n,0,n", r_order_line);

    // Product <-> Supplier (Vertical path on Column 4)
    // Midpoint Y calculation: 5 + 5 + ((27 - (5 + 5)) / 2) - (5 / 2) = 10 + 8 - 2 = 16
    Relationship *r_supplies = addRelationship(144, 16, supplier, product, "Supplies");
    addCardinalityAPI("1,n,1,1", r_supplies);

    // Product <-> Warehouse (Diagonal / Dynamic track intercepting grid cells)
    Relationship *r_stocks = addRelationship(121, 16, warehouse, product, "Stocks");
    addPropertyRelationship(r_stocks, "aisle_num", "int");
    addPropertyRelationship(r_stocks, "qty_stock", "int");
    addCardinalityAPI("1,n,0,n", r_stocks);

    // Order <-> Delivery (Vertical path on Column 1/2 span)
    Relationship *r_fulfilled = addRelationship(25, 16, order, delivery, "Ships_Via");
    addCardinalityAPI("0,1,1,1", r_fulfilled);

    // Delivery <-> Driver (Horizontal path on Row 2)
    Relationship *r_assigns = addRelationship(31, 27, driver, delivery, "Brings");
    addCardinalityAPI("0,n,1,1", r_assigns);
}
