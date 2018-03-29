package com.rhubarb_lip_sync.rhubarb_for_spine

import java.io.FileInputStream
import java.net.MalformedURLException
import java.net.URISyntaxException
import java.net.URL
import org.apache.commons.lang3.SystemUtils.IS_OS_WINDOWS
import java.nio.file.Path
import java.nio.file.Paths

// The following code is adapted from https://stackoverflow.com/a/12733172/52041

/**
 * Gets the base location of the given class.
 *
 * If the class is directly on the file system (e.g.,
 * "/path/to/my/package/MyClass.class") then it will return the base directory
 * (e.g., "file:/path/to").
 *
 * If the class is within a JAR file (e.g.,
 * "/path/to/my-jar.jar!/my/package/MyClass.class") then it will return the
 * path to the JAR (e.g., "file:/path/to/my-jar.jar").
 *
 * @param c The class whose location is desired.
 */
fun getLocation(c: Class<*>): URL {
	// Try the easy way first
	try {
		val codeSourceLocation = c.protectionDomain.codeSource.location
		if (codeSourceLocation != null) return codeSourceLocation
	} catch (e: SecurityException) {
		// Cannot access protection domain
	} catch (e: NullPointerException) {
		// Protection domain or code source is null
	}

	// The easy way failed, so we try the hard way. We ask for the class
	// itself as a resource, then strip the class's path from the URL string,
	// leaving the base path.

	// Get the class's raw resource path
	val classResource = c.getResource(c.simpleName + ".class")
		?: throw Exception("Cannot find class resource.")

	val url = classResource.toString()
	val suffix = c.canonicalName.replace('.', '/') + ".class"
	if (!url.endsWith(suffix)) throw Exception("Malformed URL.")

	// strip the class's path from the URL string
	val base = url.substring(0, url.length - suffix.length)

	var path = base

	// remove the "jar:" prefix and "!/" suffix, if present
	if (path.startsWith("jar:")) path = path.substring(4, path.length - 2)

	return URL(path)
}

/**
 * Converts the given URL to its corresponding [Path].
 *
 * @param url The URL to convert.
 * @return A file path suitable for use with e.g. [FileInputStream]
 */
fun urlToPath(url: URL): Path {
	var pathString = url.toString()

	if (pathString.startsWith("jar:")) {
		// Remove "jar:" prefix and "!/" suffix
		val index = pathString.indexOf("!/")
		pathString = pathString.substring(4, index)
	}

	try {
		if (IS_OS_WINDOWS && pathString.matches("file:[A-Za-z]:.*".toRegex())) {
			pathString = "file:/" + pathString.substring(5)
		}
		return Paths.get(URL(pathString).toURI())
	} catch (e: MalformedURLException) {
		// URL is not completely well-formed.
	} catch (e: URISyntaxException) {
		// URL is not completely well-formed.
	}

	if (pathString.startsWith("file:")) {
		// Pass through the URL as-is, minus "file:" prefix
		pathString = pathString.substring(5)
		return Paths.get(pathString)
	}
	throw IllegalArgumentException("Invalid URL: $url")
}