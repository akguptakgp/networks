# importing important libraries
import MySQLdb as mdb
# import networkx as nx

# connecting to the database
db = mdb.connect('10.5.18.69', '12CS10036', 'btech12', '12CS10036');

# making cursors to use them for queries
cur1 = db.cursor()
cur2 = db.cursor()

def solve():
	g = {}
	sql = "select AuthorID from Authors"
	cur1.execute(sql)

	totalNodes = cur1.rowcount

	for i in range(cur1.rowcount):
		row = cur1.fetchone()
		g[int(row[0])] = []


	sql = "select p1.AuthorID, p2.AuthorID from Publish as p1 join Publish as p2 on p1.PubID = p2.pubID and p1.AuthorID < p2.AuthorID"
	cur1.execute(sql)

	for i in range(cur1.rowcount):
		row = cur1.fetchone()
		if int(row[0]) not in g[int(row[1])]:
			g[int(row[0])].append(int(row[1]))
			g[int(row[1])].append(int(row[0]))



	# print g

	result = {}
	available = {}

	table_no = -1;

	for i in range(totalNodes):
		result[i] = -1
		available[i] = False

	result[0] = 0
	print "0", "0"
	for i in range(1, totalNodes):
		neighbor = g[i]
		for n in neighbor:
			if result[n] != -1:
				available[result[n]] = True

		for cr in range(totalNodes):
			if available[cr] == False:
				break

		result[i] = cr
		if cr > table_no:
			table_no = cr

		for n in neighbor:
			if result[n] != -1:
				available[result[n]] = False

		print str(i), str(cr)

	table_no += 1
	print "Total number of tables required: " + str(table_no)
	return

if __name__ == '__main__':
	solve()